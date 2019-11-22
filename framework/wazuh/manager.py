# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import fcntl
import json
import random
import re
import socket
import subprocess
import time
from collections import OrderedDict
from datetime import datetime, timezone
from os import remove, chmod
from os.path import exists, join
from shutil import Error
from typing import Dict
from xml.dom.minidom import parseString
from xml.parsers.expat import ExpatError

from wazuh import Wazuh
from wazuh import common
from wazuh import configuration
from wazuh.core.core_agent import Agent
from wazuh.core.cluster.utils import get_manager_status, get_cluster_status, manager_restart, read_cluster_config
from wazuh.exception import WazuhError, WazuhInternalError
from wazuh.results import WazuhResult, AffectedItemsWazuhResult
from wazuh.utils import previous_month, tail, load_wazuh_xml, safe_move
from wazuh.utils import process_array
from wazuh.cluster import get_node
from wazuh.rbac.decorators import expose_resources
from wazuh.configuration import get_ossec_conf

_re_logtest = re.compile(r"^.*(?:ERROR: |CRITICAL: )(?:\[.*\] )?(.*)$")
execq_lockfile = join(common.ossec_path, "var", "run", ".api_execq_lock")
cluster_enabled = not read_cluster_config()['disabled']
node_id = get_node().get('node') if cluster_enabled else 'manager'


def status():
    """ Returns the Manager processes that are running. """

    return get_manager_status()


@expose_resources(actions=['cluster:read_config'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def get_status() -> AffectedItemsWazuhResult:
    """Wrapper for status(). """
    result = AffectedItemsWazuhResult(all_msg=f"Processes status read successfully"
                                              f"{' in specified node' if node_id != 'manager' else ''}",
                                      some_msg='Could not read basic information in some nodes',
                                      none_msg=f"Could not read processes status"
                                               f"{' in specified node' if node_id != 'manager' else ''}"
                                      )
    try:
        result.affected_items.append(status())
    except WazuhError as e:
        result.add_failed_item(id_=node_id, error=e)
    result.total_affected_items=len(result.affected_items)

    return result


def __get_ossec_log_fields(log):
    regex_category = re.compile(r"^(\d\d\d\d/\d\d/\d\d\s\d\d:\d\d:\d\d)\s(\S+)(?:\[.*)?:\s(DEBUG|INFO|CRITICAL|ERROR|WARNING):(.*)$")

    match = re.search(regex_category, log)

    if match:
        date = match.group(1)
        category = match.group(2)
        type_log = match.group(3)
        description = match.group(4)

        if "rootcheck" in category:  # Unify rootcheck category
            category = "ossec-rootcheck"

    else:
        return None

    return datetime.strptime(date, '%Y/%m/%d %H:%M:%S'), category, type_log.lower(), description


@expose_resources(actions=['cluster:read_config'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def ossec_log(type_log='all', category='all', months=3, offset=0, limit=common.database_limit, sort_by=None,
              sort_ascending=True, search_text=None, complementary_search=False, search_in_fields=None, q=''):
    """Gets logs from ossec.log.

    :param type_log: Filters by log type: all, error or info.
    :param category: Filters by log category (i.e. ossec-remoted).
    :param months: Returns logs of the last n months. By default is 3 months.
    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort_by: Fields to sort the items by
    :param sort_ascending: Sort in ascending (true) or descending (false) order
    :param search_text: Text to search
    :param complementary_search: Find items without the text to search
    :param search_in_fields: Fields to search in
    :param q: Defines query to filter.
    :return: Dictionary: {'items': array of items, 'totalItems': Number of items (without applying the limit)}
    """
    result = AffectedItemsWazuhResult(all_msg=f"Logs read successfully"
                                              f"{' in specified node' if node_id != 'manager' else ''}",
                                      some_msg='Could not read logs in some nodes',
                                      none_msg=f"Could not read logs"
                                               f"{' in specified node' if node_id != 'manager' else ''}"
                                      )
    logs = []

    first_date = previous_month(months)
    statfs_error = "ERROR: statfs('******') produced error: No such file or directory"

    try:
        for line in tail(common.ossec_log, 2000):
            log_fields = __get_ossec_log_fields(line)
            if log_fields:
                log_date, log_category, level, description = log_fields

                if log_date < first_date:
                    continue

                if category != 'all':
                    if log_category:
                        if log_category != category:
                            continue
                    else:
                        continue
                # We transform local time (ossec.log) to UTC with ISO8601 maintaining time integrity
                log_line = {'timestamp': log_date.astimezone(timezone.utc),
                            'tag': log_category, 'level': level, 'description': description}

                if type_log == 'all':
                    logs.append(log_line)
                elif type_log.lower() == level.lower():
                    if "ERROR: statfs(" in line:
                        if statfs_error in logs:
                            continue
                        else:
                            logs.append(statfs_error)
                    else:
                        logs.append(log_line)
                else:
                    continue
            else:
                if logs and line and log_category == logs[-1]['tag'] and level == logs[-1]['level']:
                    logs[-1]['description'] += "\n" + line
    except WazuhError as e:
        result.add_failed_item(id_=node_id, error=e)

    data = process_array(logs, search_text=search_text, search_in_fields=search_in_fields,
                         complementary_search=complementary_search, sort_by=sort_by,
                         sort_ascending=sort_ascending, offset=offset, limit=limit, q=q)
    result.affected_items.extend(data['items'])
    result.total_affected_items = data['totalItems']

    return result


@expose_resources(actions=['cluster:read_config'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def ossec_log_summary(months=3):
    """ Summary of ossec.log.

    :param months: Check logs of the last n months. By default is 3 months.
    :return: Summary by categories.
    """
    result = AffectedItemsWazuhResult(all_msg=f"Log summarized successfully"
                                              f"{' in specified node' if node_id != 'manager' else ''}",
                                      some_msg='Could not summarize the log in some nodes',
                                      none_msg=f"Could not summarize the log"
                                               f"{' in specified node' if node_id != 'manager' else ''}"
                                      )
    categories = dict()

    first_date = previous_month(months)

    with open(common.ossec_log, errors='ignore') as f:
        lines_count = 0
        for line in f:
            if lines_count > 50000:
                break
            lines_count = lines_count + 1

            line = __get_ossec_log_fields(line)

            # Multiline logs
            if line is None:
                continue

            log_date, category, log_type, _, = line

            if log_date < first_date:
                break

            if category:
                if category in categories:
                    categories[category]['all'] += 1
                else:
                    categories[category] = {'all': 1, 'info': 0, 'error': 0, 'critical': 0, 'warning': 0, 'debug': 0}
                categories[category][log_type] += 1
            else:
                continue

    for k, v in categories.items():
        result.affected_items.append({k: v})
    result.affected_items = sorted(result.affected_items, key=lambda i: list(i.keys())[0])
    result.total_affected_items = len(result.affected_items)

    return result


@expose_resources(actions=['cluster:upload_file'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def upload_file(path=None, content=None, overwrite=False):
    """Upload a new file

    :param path: Path of destination of the new file
    :param content: Content of file to be uploaded
    :param overwrite: True for updating existing files, False otherwise
    """
    result = AffectedItemsWazuhResult(all_msg='File was uploaded successfully',
                                      none_msg='Could not upload file'
                                      )
    try:
        # If file already exists and overwrite is False, raise exception
        if not overwrite and exists(join(common.ossec_path, path)):
            raise WazuhError(1905)
        elif overwrite and exists(join(common.ossec_path, path)):
            delete_file(path=path)
        if len(content) == 0:
            raise WazuhError(1112)

        # For CDB lists
        if re.match(r'^etc/lists', path):
            upload_list(content, path)
        else:
            upload_xml(content, path)
        result.affected_items.append(path)
    except WazuhError as e:
        result.add_failed_item(id_=path, error=e)
    result.total_affected_items = len(result.affected_items)

    return result


def upload_xml(xml_file, path):
    """
    Updates XML files (rules and decoders)
    :param xml_file: content of the XML file
    :param path: Destination of the new XML file
    :return: Confirmation message
    """
    # path of temporary files for parsing xml input
    tmp_file_path = '{}/tmp/api_tmp_file_{}_{}.xml'.format(common.ossec_path, time.time(), random.randint(0, 1000))

    # create temporary file for parsing xml input
    try:
        with open(tmp_file_path, 'w') as tmp_file:
            # beauty xml file
            xml = parseString('<root>' + xml_file + '</root>')
            # remove first line (XML specification: <? xmlversion="1.0" ?>), <root> and </root> tags, and empty lines
            indent = '  '  # indent parameter for toprettyxml function
            pretty_xml = '\n'.join(filter(lambda x: x.strip(), xml.toprettyxml(indent=indent).split('\n')[2:-2])) + '\n'
            # revert xml.dom replacings
            # (https://github.com/python/cpython/blob/8e0418688906206fe59bd26344320c0fc026849e/Lib/xml/dom/minidom.py#L305)
            pretty_xml = pretty_xml.replace("&amp;", "&").replace("&lt;", "<").replace("&quot;", "\"", ) \
                .replace("&gt;", ">").replace('&apos;', "'")
            # delete two first spaces of each line
            final_xml = re.sub(fr'^{indent}', '', pretty_xml, flags=re.MULTILINE)
            tmp_file.write(final_xml)
        chmod(tmp_file_path, 0o660)
    except IOError:
        raise WazuhInternalError(1005)
    except ExpatError:
        raise WazuhError(1113)

    try:
        # check xml format
        try:
            load_wazuh_xml(tmp_file_path)
        except Exception as e:
            raise WazuhError(1113, str(e))

        # move temporary file to group folder
        try:
            new_conf_path = join(common.ossec_path, path)
            safe_move(tmp_file_path, new_conf_path, permissions=0o660)
        except Error:
            raise WazuhInternalError(1016)

        return WazuhResult({'message': 'File updated successfully'})

    except Exception as e:
        # remove created temporary file if an exception happens
        remove(tmp_file_path)
        raise e


def upload_list(list_file, path):
    """
    Updates CDB lists
    :param list_file: content of the list
    :param path: Destination of the new list file
    :return: Confirmation message.
    """
    # path of temporary file
    tmp_file_path = '{}/tmp/api_tmp_file_{}_{}.txt'.format(common.ossec_path, time.time(), random.randint(0, 1000))

    try:
        # create temporary file
        with open(tmp_file_path, 'w') as tmp_file:
            # write json in tmp_file_path
            for element in list_file.splitlines():
                # skip empty lines
                if not element:
                    continue
                tmp_file.write(element.strip() + '\n')
        chmod(tmp_file_path, 0o640)
    except IOError:
        raise WazuhInternalError(1005)

    # validate CDB list
    if not validate_cdb_list(tmp_file_path):
        raise WazuhError(1802)

    # move temporary file to group folder
    try:
        new_conf_path = join(common.ossec_path, path)
        safe_move(tmp_file_path, new_conf_path, permissions=0o660)
    except Error:
        raise WazuhInternalError(1016)

    return WazuhResult({'message': 'File updated successfully'})

@expose_resources(actions=['cluster:read_file'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*',
                                                            'file:path:{path}'])
def get_file(path, validate=False):
    """Returns the content of a file.

    :param path: Relative path of file from origin
    :param validate: Whether to validate file content or not
    :return: Content file.
    """
    full_path = join(common.ossec_path, path[0])

    # validate CDB lists files
    if validate and re.match(r'^etc/lists', path[0]) and not validate_cdb_list(path[0]):
        raise WazuhError(1800, {'path': path[0]})

    # validate XML files
    if validate and not validate_xml(path[0]):
        raise WazuhError(1113)

    # check if file exists
    if not exists(full_path):
        raise WazuhError(1006)

    try:
        with open(full_path) as f:
            output = f.read()
    except IOError:
        raise WazuhInternalError(1005)

    return WazuhResult({'contents': output})


def validate_xml(path):
    """
    Validates a XML file
    :param path: Relative path of file from origin
    :return: True if XML is OK, False otherwise
    """
    full_path = join(common.ossec_path, path)
    try:
        with open(full_path) as f:
            parseString('<root>' + f.read() + '</root>')
    except IOError:
        raise WazuhInternalError(1005)
    except ExpatError:
        return False

    return True


def validate_cdb_list(path):
    """
    Validates a CDB list
    :param path: Relative path of file from origin
    :return: True if CDB list is OK, False otherwise
    """
    full_path = join(common.ossec_path, path)
    regex_cdb = re.compile(r'^[^:]+:[^:]*$')
    try:
        with open(full_path) as f:
            for line in f:
                # skip empty lines
                if not line.strip():
                    continue
                if not re.match(regex_cdb, line):
                    return False
    except IOError:
        raise WazuhInternalError(1005)

    return True


@expose_resources(actions=['cluster:delete_file'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*',
                                                              'file:path:{path}'])
def delete_file(path):
    """Deletes a file.

    :param path: Relative path of the file to be deleted
    """
    result = AffectedItemsWazuhResult(all_msg='File was deleted successfully',
                                      none_msg='Could not delete file'
                                      )

    full_path = join(common.ossec_path, path[0])

    try:
        if exists(full_path):
            try:
                remove(full_path)
                result.affected_items.append(path)
            except IOError:
                raise WazuhError(1907)
        else:
            raise WazuhError(1906)
    except WazuhError as e:
        result.add_failed_item(id_=path[0], error=e)
    result.total_affected_items = len(result.affected_items)

    return result


@expose_resources(actions=['cluster:restart'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def restart():
    """Wrapper for 'restart_manager' function due to interdependencies with cluster module and permission access. """
    result = AffectedItemsWazuhResult(all_msg=f"Restart request sent to"
                                              f"{' all specified nodes' if node_id != ' manager' else ''}",
                                      some_msg='Could not send restart request to some specified nodes',
                                      none_msg=f"No restart request sent",
                                      sort_casting=['str']
                                      )
    try:
        manager_restart()
        result.affected_items.append(node_id)
    except WazuhError as e:
        result.add_failed_item(id_=node_id, error=e)
    result.total_affected_items = len(result.affected_items)

    return result


def _check_wazuh_xml(files):
    """Check Wazuh XML format from a list of files.

    :param files: List of files to check.
    :return: None
    """
    for f in files:
        try:
            subprocess.check_output(['{}/bin/verify-agent-conf'.format(common.ossec_path), '-f', f],
                                    stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as e:
            # extract error message from output. Example of raw output 2019/01/08 14:51:09 verify-agent-conf: ERROR:
            # (1230): Invalid element in the configuration: 'agent_conf'.\n2019/01/08 14:51:09 verify-agent-conf:
            # ERROR: (1207): Syscheck remote configuration in
            # '/var/ossec/tmp/api_tmp_file_2019-01-08-01-1546959069.xml' is corrupted.\n\n Example of desired output:
            # Invalid element in the configuration: 'agent_conf'. Syscheck remote configuration in
            # '/var/ossec/tmp/api_tmp_file_2019-01-08-01-1546959069.xml' is corrupted.
            output_regex = re.findall(pattern=r"\d{4}\/\d{2}\/\d{2} \d{2}:\d{2}:\d{2} verify-agent-conf: ERROR: "
                                              r"\(\d+\): ([\w \/ \_ \- \. ' :]+)", string=e.output.decode())
            raise WazuhError(1114, ' '.join(output_regex))
        except Exception as e:
            raise WazuhError(1743, str(e))


@expose_resources(actions=['cluster:read_config'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def validation():
    """Check if Wazuh configuration is OK.

    :return: Confirmation message.
    """
    result = AffectedItemsWazuhResult(all_msg=f"Validation checked successfully"
                                              f"{' in all nodes' if node_id != 'manager' else ''}",
                                      some_msg='Could not check validation in some nodes',
                                      none_msg=f"Could not check validation"
                                               f"{' in any node' if node_id != 'manager' else ''}",
                                      sort_fields=['name'],
                                      sort_casting=['str']
                                      )

    lock_file = open(execq_lockfile, 'a+')
    fcntl.lockf(lock_file, fcntl.LOCK_EX)
    try:
        # Sockets path
        api_socket_relative_path = join('queue', 'alerts', 'execa')
        api_socket_path = join(common.ossec_path, api_socket_relative_path)
        execq_socket_path = common.EXECQ
        # Message for checking Wazuh configuration
        execq_msg = 'check-manager-configuration '

        # Remove api_socket if exists
        try:
            remove(api_socket_path)
        except OSError as e:
            if exists(api_socket_path):
                extra_msg = f'Socket: WAZUH_PATH/{api_socket_relative_path}. Error: {e.strerror}'
                raise WazuhInternalError(1014, extra_message=extra_msg)

        # up API socket
        try:
            api_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
            api_socket.bind(api_socket_path)
            # Timeout
            api_socket.settimeout(5)
        except OSError as e:
            extra_msg = f'Socket: WAZUH_PATH/{api_socket_relative_path}. Error: {e.strerror}'
            raise WazuhInternalError(1013, extra_message=extra_msg)

        # Connect to execq socket
        if exists(execq_socket_path):
            try:
                execq_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
                execq_socket.connect(execq_socket_path)
            except OSError as e:
                extra_msg = f'Socket: WAZUH_PATH/queue/alerts/execq. Error {e.strerror}'
                raise WazuhInternalError(1013, extra_message=extra_msg)
        else:
            raise WazuhInternalError(1901)

        # Send msg to execq socket
        try:
            execq_socket.send(execq_msg.encode())
            execq_socket.close()
        except socket.error as e:
            raise WazuhInternalError(1014, extra_message=str(e))
        finally:
            execq_socket.close()

        # If api_socket receives a message, configuration is OK
        try:
            buffer = bytearray()
            # Receive data
            datagram = api_socket.recv(4096)
            buffer.extend(datagram)
        except socket.timeout as e:
            raise WazuhInternalError(1014, extra_message=str(e))
        finally:
            api_socket.close()
            # Remove api_socket
            if exists(api_socket_path):
                remove(api_socket_path)

        try:
            response = _parse_execd_output(buffer.decode('utf-8').rstrip('\0'))
        except (KeyError, json.decoder.JSONDecodeError) as e:
            raise WazuhInternalError(1904, extra_message=str(e))

        result.affected_items.append({'name': node_id, **response})
        result.total_affected_items += 1
    except WazuhError as e:
        result.add_failed_item(id_=node_id, error=e)
    finally:
        fcntl.lockf(lock_file, fcntl.LOCK_UN)
        lock_file.close()

    return result


def _parse_execd_output(output: str) -> Dict:
    """
    Parses output from execd socket to fetch log message and remove log date, log daemon, log level, etc.
    :param output: Raw output from execd
    :return: Cleaned log message in a dictionary structure
    """
    json_output = json.loads(output)
    error_flag = json_output['error']
    if error_flag != 0:
        errors = []
        log_lines = json_output['message'].splitlines(keepends=False)
        for line in log_lines:
            match = _re_logtest.match(line)
            if match:
                errors.append(match.group(1))
        errors = list(OrderedDict.fromkeys(errors))
        raise WazuhError(1908, extra_message=', '.join(errors))
    else:
        response = {'status': 'OK'}

    return response


@expose_resources(actions=['cluster:read_config'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def get_config(component=None, config=None):
    """ Wrapper for get_active_configuration

    :param component: Selected component.
    :param config: Configuration to get, written on disk.
    """
    result = AffectedItemsWazuhResult(all_msg=f"Active configuration read successfully"
                                              f"{' in specified node' if node_id != 'manager' else ''}",
                                      some_msg='Could not read active configuration in some nodes',
                                      none_msg=f"Could not read active configuration"
                                               f"{' in specified node' if node_id != 'manager' else ''}"
                                      )

    try:
        data = configuration.get_active_configuration(agent_id='000', component=component, configuration=config)
        len(data.keys()) > 0 and result.affected_items.append(data)
    except WazuhError as e:
        result.add_failed_item(id_=node_id, error=e)
    result.total_affected_items = len(result.affected_items)

    return result


def get_info() -> Dict:
    """
    Returns manager configuration with cluster details

    :return: Dictionary with information about manager and cluster
    """
    # get name from agent 000
    manager = Agent(id=0)
    manager.load_info_from_db()

    # read cluster configuration
    cluster_config = read_cluster_config()

    # get manager status
    cluster_info = get_cluster_status()
    # add 'name', 'node_name' and 'node_type' to cluster_info
    for name in ('name', 'node_name', 'node_type'):
        cluster_info[name] = cluster_config[name]

    # merge manager information into an unique dictionary
    manager_info = {**Wazuh().to_dict(),
                    **{'name': manager.name, 'cluster': cluster_info}}

    return manager_info


@expose_resources(actions=['cluster:read_config'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def read_ossec_conf(section=None, field=None):
    """ Wrapper for get_ossec_conf

    :param section: Filters by section (i.e. rules).
    :param field: Filters by field in section (i.e. included).
    """
    result = AffectedItemsWazuhResult(all_msg=f"Configuration read successfully"
                                              f"{' in specified node' if node_id != 'manager' else ''}",
                                      some_msg='Could not read configuration in some nodes',
                                      none_msg=f"Could not read configuration"
                                               f"{' in specified node' if node_id != 'manager' else ''}"
                                      )

    try:
        result.affected_items.append(get_ossec_conf(section=section, field=field))
    except WazuhError as e:
        result.add_failed_item(id_=node_id, error=e)
    result.total_affected_items = len(result.affected_items)

    return result


@expose_resources(actions=['cluster:read_config'], resources=[f'node:id:{node_id}' if cluster_enabled else '*:*:*'])
def get_basic_info():
    """ Wrapper for Wazuh().to_dict"""
    result = AffectedItemsWazuhResult(all_msg=f"Basic information read successfully"
                                              f"{' in specified node' if node_id != 'manager' else ''}",
                                      some_msg='Could not read basic information in some nodes',
                                      none_msg=f"Could not read basic information"
                                               f"{' in specified node' if node_id != 'manager' else ''}"
                                      )

    try:
        result.affected_items.append(Wazuh().to_dict())
    except WazuhError as e:
        result.add_failed_item(id_=node_id, error=e)
    result.total_affected_items = len(result.affected_items)

    return result
