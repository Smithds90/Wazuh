# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2
from wazuh.cluster import local_client
from wazuh import common, exception
import json


async def get_nodes(filter_node=None, offset=0, limit=common.database_limit, sort=None, search=None, select=None, filter_type='all'):
    arguments = {'filter_node': filter_node, 'offset': offset, 'limit': limit, 'sort': sort, 'search': search,
                 'select': select, 'filter_type': filter_type}
    return json.loads(await local_client.execute(command=b'get_nodes', data=json.dumps(arguments).encode(),
                                                 wait_for_complete=False))


async def get_node(filter_node=None):
    arguments = {'filter_node': filter_node, 'offset': 0, 'limit': common.database_limit, 'sort': None, 'search': None,
                 'select': None, 'filter_type': 'all'}
    node_info_array = json.loads(await local_client.execute(command=b'get_nodes', data=json.dumps(arguments).encode(),
                                                            wait_for_complete=False))
    if len(node_info_array['items']) > 0:
        return node_info_array['items'][0]
    else:
        return {}


async def get_health(filter_node=None):
    return json.loads(await local_client.execute(command=b'get_health', data=json.dumps(filter_node).encode()))


async def get_agents(filter_node=None, filter_status=None):
    filter_status = ["all"] if not filter_status else filter_status
    filter_node = ["all"] if not filter_node else filter_node

    input_json = {'function': '/agents', 'from_cluster': False,
                  'arguments': {
                      'filters': {'status': ','.join(filter_status), 'node_name': ','.join(filter_node)},
                      'limit': None, 'wait_for_complete': False,
                      'select': {'fields': ['id', 'ip', 'name', 'status', 'node_name', 'version']}}}

    result = json.loads(await local_client.execute(command=b'dapi', data=json.dumps(input_json).encode(),
                                                   wait_for_complete=False))
    if result['error'] > 0:
        raise exception.WazuhException(result['error'], result['message'])
    return result

