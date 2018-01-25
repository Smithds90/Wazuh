#!/usr/bin/env python

# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

from wazuh.utils import cut_array, sort_array, search_array, plain_dict_to_nested_dict, chmod_r, chown_r, create_exception_dic
from wazuh.InputValidator import InputValidator
from wazuh.exception import WazuhException
from wazuh.database import Connection
from wazuh import common
from os import path, listdir, chmod
from time import time
from shutil import move, copytree
from glob import glob
import hashlib
from operator import setitem
from pwd import getpwnam
from grp import getgrnam

def _remove_single_group(group_id):
    """
    Remove the group in every agent.

    :param group_id: Group ID.
    :return: Confirmation message.
    """

    if group_id.lower() == "default":
        raise WazuhException(1712)

    if not group_exists(group_id):
        raise WazuhException(1710, group_id)

    ids = []

    # Remove agent group
    agents = get_agent_group(group_id=group_id, limit=None)
    for agent in agents['items']:
        unset_group(agent['id'])
        ids.append(agent['id'])

    # Remove group directory
    group_path = "{0}/{1}".format(common.shared_path, group_id)
    group_backup = "{0}/groups/{1}_{2}".format(common.backup_path, group_id, int(time()))
    if path.exists(group_path):
        move(group_path, group_backup)

    msg = "Group '{0}' removed.".format(group_id)

    return {'msg': msg, 'affected_agents': ids}


def get_all_groups_sql(offset=0, limit=common.database_limit, sort=None, search=None):
    """
    Gets the existing groups.

    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort: Sorts the items. Format: {"fields":["field1","field2"],"order":"asc|desc"}.
    :param search: Looks for items with the specified string.
    :return: Dictionary: {'items': array of items, 'totalItems': Number of items (without applying the limit)}
    """

    # Connect DB
    db_global = glob(common.database_path_global)
    if not db_global:
        raise WazuhException(1600)

    conn = Connection(db_global[0])

    # Init query
    query = "SELECT DISTINCT {0} FROM agent WHERE `group` IS NOT null"
    fields = {'name': 'group'}  # field: db_column
    select = ["`group`"]
    request = {}

    # Search
    if search:
        query += " AND NOT" if bool(search['negation']) else ' AND'
        query += " ( `group` LIKE :search )"
        request['search'] = '%{0}%'.format(search['value'])

    # Count
    conn.execute(query.format('COUNT(DISTINCT `group`)'), request)
    data = {'totalItems': conn.fetch()[0]}

    # Sorting
    if sort:
        if sort['fields']:
            allowed_sort_fields = fields.keys()
            # Check if every element in sort['fields'] is in allowed_sort_fields.
            if not set(sort['fields']).issubset(allowed_sort_fields):
                raise WazuhException(1403, 'Allowed sort fields: {0}. Fields: {1}'.format(allowed_sort_fields, sort['fields']))

            order_str_fields = ['`{0}` {1}'.format(fields[i], sort['order']) for i in sort['fields']]
            query += ' ORDER BY ' + ','.join(order_str_fields)
        else:
            query += ' ORDER BY `group` {0}'.format(sort['order'])
    else:
        query += ' ORDER BY `group` ASC'

    # OFFSET - LIMIT
    if limit:
        query += ' LIMIT :offset,:limit'
        request['offset'] = offset
        request['limit'] = limit

    # Data query
    conn.execute(query.format(','.join(select)), request)

    data['items'] = []

    for tuple in conn:
        if tuple[0] != None:
            data['items'].append(tuple[0])

    return data


def get_all_groups(offset=0, limit=common.database_limit, sort=None, search=None, hash_algorithm='md5'):
    """
    Gets the existing groups.

    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort: Sorts the items. Format: {"fields":["field1","field2"],"order":"asc|desc"}.
    :param search: Looks for items with the specified string.
    :return: Dictionary: {'items': array of items, 'totalItems': Number of items (without applying the limit)}
    """
    def get_hash(file, hash_algorithm='md5'):
        filename = "{0}/{1}".format(common.shared_path, file)

        # check hash algorithm
        try:
            algorithm_list = hashlib.algorithms_available
        except Exception as e:
            algorithm_list = hashlib.algorithms

        if not hash_algorithm in algorithm_list:
            raise WazuhException(1723, "Available algorithms are {0}.".format(algorithm_list))

        hashing = hashlib.new(hash_algorithm)

        try:
            with open(filename, 'rb') as f:
                hashing.update(f.read())
        except IOError:
            return None

        return hashing.hexdigest()

    # Connect DB
    db_global = glob(common.database_path_global)
    if not db_global:
        raise WazuhException(1600)

    conn = Connection(db_global[0])
    query = "SELECT {0} FROM agent WHERE `group` = :group_id"

    # Group names
    data = []
    for entry in listdir(common.shared_path):
        full_entry = path.join(common.shared_path, entry)
        if not path.isdir(full_entry):
            continue

        # Group count
        request = {'group_id': entry}
        conn.execute(query.format('COUNT(*)'), request)

        # merged.mg and agent.conf sum
        merged_sum = get_hash(entry + "/merged.mg")
        conf_sum   = get_hash(entry + "/agent.conf")

        item = {'count':conn.fetch()[0], 'name': entry, 'file_count': len(listdir(full_entry))}

        if merged_sum:
            item['merged_sum'] = merged_sum

        if conf_sum:
            item['conf_sum'] = conf_sum

        data.append(item)


    if search:
        data = search_array(data, search['value'], search['negation'], fields=['name'])

    if sort:
        data = sort_array(data, sort['fields'], sort['order'])
    else:
        data = sort_array(data, ['name'])

    return {'items': cut_array(data, offset, limit), 'totalItems': len(data)}


def group_exists_sql(group_id):
    """
    Checks if the group exists

    :param group_id: Group ID.
    :return: True if group exists, False otherwise
    """
    # Input Validation of group_id
    if not InputValidator().group(group_id):
        raise WazuhException(1722)

    db_global = glob(common.database_path_global)
    if not db_global:
        raise WazuhException(1600)

    conn = Connection(db_global[0])

    query = "SELECT `group` FROM agent WHERE `group` = :group_id LIMIT 1"
    request = {'group_id': group_id}

    conn.execute(query, request)

    for tuple in conn:

        if tuple[0] != None:
            return True
        else:
            return False


def group_exists(group_id):
    """
    Checks if the group exists

    :param group_id: Group ID.
    :return: True if group exists, False otherwise
    """
    # Input Validation of group_id
    if not InputValidator().group(group_id):
        raise WazuhException(1722)

    if path.exists("{0}/{1}".format(common.shared_path, group_id)):
        return True
    else:
        return False


def get_agent_group(group_id, offset=0, limit=common.database_limit, sort=None, search=None, select=None):
    """
    Gets the agents in a group

    :param group_id: Group ID.
    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort: Sorts the items. Format: {"fields":["field1","field2"],"order":"asc|desc"}.
    :param search: Looks for items with the specified string.
    :return: Dictionary: {'items': array of items, 'totalItems': Number of items (without applying the limit)}
    """

    # Connect DB
    db_global = glob(common.database_path_global)
    if not db_global:
        raise WazuhException(1600)

    conn = Connection(db_global[0])
    valid_select_fiels = ["id", "name", "ip", "last_keepalive", "os_name",
                         "os_version", "os_platform", "os_uname", "version",
                         "config_sum", "merged_sum", "manager_host"]
    search_fields = {"id", "name", "os_name"}

    # Init query
    query = "SELECT {0} FROM agent WHERE `group` = :group_id"
    fields = {'id': 'id', 'name': 'name'}  # field: db_column
    request = {'group_id': group_id}

    # Select
    if select:
        if not set(select['fields']).issubset(valid_select_fiels):
            uncorrect_fields = map(lambda x: str(x), set(select['fields']) - set(valid_select_fiels))
            raise WazuhException(1724, "Allowed select fields: {0}. Fields {1}".\
                    format(valid_select_fiels, uncorrect_fields))
        select_fields = select['fields']
    else:
        select_fields = valid_select_fiels

    # Search
    if search:
        query += " AND NOT" if bool(search['negation']) else ' AND'
        query += " (" + " OR ".join(x + ' LIKE :search' for x in search_fields) + " )"
        request['search'] = '%{0}%'.format(int(search['value']) if search['value'].isdigit()
                                                                else search['value'])

    # Count
    conn.execute(query.format('COUNT(*)'), request)
    data = {'totalItems': conn.fetch()[0]}

    # Sorting
    if sort:
        if sort['fields']:
            allowed_sort_fields = select_fields
            # Check if every element in sort['fields'] is in allowed_sort_fields.
            if not set(sort['fields']).issubset(allowed_sort_fields):
                raise WazuhException(1403, 'Allowed sort fields: {0}. Fields: {1}'.\
                    format(allowed_sort_fields, sort['fields']))

            order_str_fields = ['{0} {1}'.format(fields[i], sort['order']) for i in sort['fields']]
            query += ' ORDER BY ' + ','.join(order_str_fields)
        else:
            query += ' ORDER BY id {0}'.format(sort['order'])
    else:
        query += ' ORDER BY id ASC'

    # OFFSET - LIMIT
    if limit:
        query += ' LIMIT :offset,:limit'
        request['offset'] = offset
        request['limit'] = limit

    # Data query
    conn.execute(query.format(','.join(select_fields)), request)

    non_nested = [{field:tuple_elem for field,tuple_elem \
            in zip(select_fields, tuple) if tuple_elem} for tuple in conn]
    map(lambda x: setitem(x, 'id', str(x['id']).zfill(3)), non_nested)

    data['items'] = [plain_dict_to_nested_dict(d, ['os']) for d in non_nested]

    return data


def get_group_files(group_id=None, offset=0, limit=common.database_limit, sort=None, search=None):
    """
    Gets the group files.

    :param group_id: Group ID.
    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort: Sorts the items. Format: {"fields":["field1","field2"],"order":"asc|desc"}.
    :param search: Looks for items with the specified string.
    :return: Dictionary: {'items': array of items, 'totalItems': Number of items (without applying the limit)}
    """

    group_path = common.shared_path
    if group_id:
        if not group_exists(group_id):
            raise WazuhException(1710, group_id)
        group_path = "{0}/{1}".format(common.shared_path, group_id)

    if not path.exists(group_path):
        raise WazuhException(1006, group_path)

    try:
        data = []
        for entry in listdir(group_path):
            item = {}
            try:
                item['filename'] = entry
                with open("{0}/{1}".format(group_path, entry), 'rb') as f:
                    item['hash'] = hashlib.md5(f.read()).hexdigest()
                data.append(item)
            except (OSError, IOError) as e:
                pass

        try:
            # ar.conf
            ar_path = "{0}/ar.conf".format(common.shared_path, entry)
            with open(ar_path, 'rb') as f:
                hash_ar = hashlib.md5(f.read()).hexdigest()
            data.append({'filename': "ar.conf", 'hash': hash_ar})
        except (OSError, IOError) as e:
            pass

        if search:
            data = search_array(data, search['value'], search['negation'])

        if sort:
            data = sort_array(data, sort['fields'], sort['order'])
        else:
            data = sort_array(data, ["filename"])

        return {'items': cut_array(data, offset, limit), 'totalItems': len(data)}
    except Exception as e:
        raise WazuhException(1727, str(e))


def create_group(group_id):
    """
    Creates a group.

    :param group_id: Group ID.
    :return: Confirmation message.
    """
    # Input Validation of group_id
    if not InputValidator().group(group_id):
        raise WazuhException(1722)

    group_path = "{0}/{1}".format(common.shared_path, group_id)

    if group_id.lower() == "default" or path.exists(group_path):
        raise WazuhException(1711, group_id)

    ossec_uid = getpwnam("ossec").pw_uid
    ossec_gid = getgrnam("ossec").gr_gid

    # Create group in /etc/shared
    group_def_path = "{0}/default".format(common.shared_path)
    try:
        copytree(group_def_path, group_path)
        chown_r(group_path, ossec_uid, ossec_gid)
        chmod_r(group_path, 0o660)
        chmod(group_path, 0o770)
        msg = "Group '{0}' created.".format(group_id)
    except Exception as e:
        raise WazuhException(1005, str(e))

    return msg


def remove_group(group_id):
    """
    Remove the group in every agent.

    :param group_id: Group ID.
    :return: Confirmation message.
    """

    # Input Validation of group_id
    if not InputValidator().group(group_id):
        raise WazuhException(1722)


    failed_ids = []
    ids = []
    affected_agents = []
    if isinstance(group_id, list):
        for id in group_id:

            if id.lower() == "default":
                raise WazuhException(1712)

            try:
                removed = _remove_single_group(id)
                ids.append(id)
                affected_agents += removed['affected_agents']
            except WazuhException as e:
                failed_ids.append(create_exception_dic(id, e))
    else:
        if group_id.lower() == "default":
            raise WazuhException(1712)

        try:
            removed = _remove_single_group(group_id)
            ids.append(group_id)
            affected_agents += removed['affected_agents']
        except WazuhException as e:
            failed_ids.append(create_exception_dic(group_id, e))

    final_dict = {}
    if not failed_ids:
        message = 'All selected groups were removed'
        final_dict = {'msg': message, 'ids': ids, 'affected_agents': affected_agents}
    else:
        message = 'Some groups were not removed'
        final_dict = {'msg': message, 'failed_ids': failed_ids, 'ids': ids, 'affected_agents': affected_agents}

    return final_dict

