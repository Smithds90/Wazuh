# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import asyncio
import logging

import connexion

from api.authentication import get_permissions
from api.util import remove_nones_to_dict, exception_handler, parse_api_param, raise_if_exc, flask_cached
from wazuh import rule as rule_framework
from wazuh.cluster.dapi.dapi import DistributedAPI

loop = asyncio.get_event_loop()
logger = logging.getLogger('wazuh')


@exception_handler
@flask_cached
def get_rules(rule_ids=None, pretty=False, wait_for_complete=False, offset=0, limit=None, sort=None, search=None,
              status=None, group=None, level=None, file=None, path=None, pci=None, gdpr=None, gpg13=None, hipaa=None):
    """Get information about all Wazuh rules.

    :param rule_ids: Filters by rule ID
    :param pretty: Show results in human-readable format
    :param wait_for_complete: Disable timeout response
    :param offset: First element to return in the collection
    :param limit: Maximum number of elements to return
    :param sort: Sorts the collection by a field or fields (separated by comma). Use +/- at the beginning to list in
    ascending or descending order.
    :param search: Looks for elements with the specified string
    :param status: Filters by rules status.
    :param group: Filters by rule group.
    :param level: Filters by rule level. Can be a single level (4) or an interval (2-4)
    :param file: Filters by filename.
    :param path: Filters by rule path.
    :param pci: Filters by PCI requirement name.
    :param gdpr: Filters by GDPR requirement.
    :param gpg13: Filters by GPG13 requirement.
    :param hipaa: Filters by HIPAA requirement.
    :return: Data object
    """
    f_kwargs = {'rule_ids': rule_ids, 'offset': offset, 'limit': limit,
                'sort_by': parse_api_param(sort, 'sort')['fields'] if sort is not None else ['id'],
                'sort_ascending': True if sort is None or parse_api_param(sort, 'sort')['order'] == 'asc' else False,
                'search_text': parse_api_param(search, 'search')['value'] if search is not None else None,
                'complementary_search': parse_api_param(search, 'search')['negation'] if search is not None else None,
                'status': status, 'group': group, 'level': level, 'file': file, 'path': path, 'pci': pci, 'gdpr': gdpr,
                'gpg13': gpg13, 'hipaa': hipaa, 'nist_800_53': connexion.request.args.get('nist-800-53', None)}

    dapi = DistributedAPI(f=rule_framework.get_rules,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_any',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          pretty=pretty,
                          logger=logger,
                          rbac_permissions=get_permissions(connexion.request.headers['Authorization'])
                          )
    data = raise_if_exc(loop.run_until_complete(dapi.distribute_function()))

    return data, 200


@exception_handler
@flask_cached
def get_rules_groups(pretty=False, wait_for_complete=False, offset=0, limit=None, sort=None, search=None):
    """Get all rule groups names.

    :param pretty: Show results in human-readable format
    :param wait_for_complete: Disable timeout response
    :param offset: First element to return in the collection
    :param limit: Maximum number of elements to return
    :param sort: Sorts the collection by a field or fields (separated by comma). Use +/- at the beginning to list in
    ascending or descending order.
    :param search: Looks for elements with the specified string
    :return: Data object
    """
    f_kwargs = {'offset': offset,
                'limit': limit,
                'sort_by': parse_api_param(sort, 'sort')['fields'] if sort is not None else [''],
                'sort_ascending': True if sort is None or parse_api_param(sort, 'sort')['order'] == 'asc' else False,
                'search_text': parse_api_param(search, 'search')['value'] if search is not None else None,
                'complementary_search': parse_api_param(search, 'search')['negation'] if search is not None else None,
                }

    dapi = DistributedAPI(f=rule_framework.get_groups,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_any',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          pretty=pretty,
                          logger=logger,
                          rbac_permissions=get_permissions(connexion.request.headers['Authorization'])
                          )
    data = raise_if_exc(loop.run_until_complete(dapi.distribute_function()))

    return data, 200


@exception_handler
@flask_cached
def get_rules_requirement(requirement=None, pretty=False, wait_for_complete=False, offset=0, limit=None, sort=None,
                          search=None):
    """Get all PCI requirements

    :param requirement: Get the specified requirement in all rules in the system.
    :param pretty: Show results in human-readable format
    :param wait_for_complete: Disable timeout response
    :param offset: First element to return in the collection
    :param limit: Maximum number of elements to return
    :param sort: Sorts the collection by a field or fields (separated by comma). Use +/- at the beginning to list in
    ascending or descending order.
    :param search: Looks for elements with the specified string
    :return: Data object
    """
    f_kwargs = {'requirement': requirement.replace('-', '_'), 'offset': offset, 'limit': limit,
                'sort_by': parse_api_param(sort, 'sort')['fields'] if sort is not None else [''],
                'sort_ascending': True if sort is None or parse_api_param(sort, 'sort')['order'] == 'asc' else False,
                'search_text': parse_api_param(search, 'search')['value'] if search is not None else None,
                'complementary_search': parse_api_param(search, 'search')['negation'] if search is not None else None}

    dapi = DistributedAPI(f=rule_framework.get_requirement,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_any',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          pretty=pretty,
                          logger=logger,
                          rbac_permissions=get_permissions(connexion.request.headers['Authorization'])
                          )
    data = raise_if_exc(loop.run_until_complete(dapi.distribute_function()))

    return data, 200


@exception_handler
@flask_cached
def get_rules_files(pretty=False, wait_for_complete=False, offset=0, limit=None, sort=None, search=None,
                    status=None, file=None, path=None):
    """Get all files which defines rules

    :param pretty: Show results in human-readable format
    :param wait_for_complete: Disable timeout response
    :param offset: First element to return in the collection
    :param limit: Maximum number of elements to return
    :param sort: Sorts the collection by a field or fields (separated by comma). Use +/- at the beginning to list in
    ascending or descending order.
    :param search: Looks for elements with the specified string
    :param status: Filters by rules status.
    :param file: Filters by filename.
    :param path: Filters by rule path.
    :return: Data object
    """
    f_kwargs = {'offset': offset, 'limit': limit,
                'sort_by': parse_api_param(sort, 'sort')['fields'] if sort is not None else ['file'],
                'sort_ascending': True if sort is None or parse_api_param(sort, 'sort')['order'] == 'asc' else False,
                'search_text': parse_api_param(search, 'search')['value'] if search is not None else None,
                'complementary_search': parse_api_param(search, 'search')['negation'] if search is not None else None,
                'status': status, 'file': file,
                'path': path}

    dapi = DistributedAPI(f=rule_framework.get_rules_files,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_any',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          pretty=pretty,
                          logger=logger,
                          rbac_permissions=get_permissions(connexion.request.headers['Authorization'])
                          )
    data = raise_if_exc(loop.run_until_complete(dapi.distribute_function()))

    return data, 200


@exception_handler
@flask_cached
def get_download_file(pretty: bool = False, wait_for_complete: bool = False, file: str = None):
    """Download an specified decoder file.

    :param pretty: Show results in human-readable format
    :param wait_for_complete: Disable timeout response
    :param file: File name to download.
    :return: Raw XML file
    """
    f_kwargs = {'filename': file}

    dapi = DistributedAPI(f=rule_framework.get_file,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_any',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          pretty=pretty,
                          logger=logger,
                          rbac_permissions=get_permissions(connexion.request.headers['Authorization'])
                          )
    data = raise_if_exc(loop.run_until_complete(dapi.distribute_function()))
    response = connexion.lifecycle.ConnexionResponse(body=data["message"], mimetype='application/xml')

    return response
