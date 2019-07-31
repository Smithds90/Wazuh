# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import re
from functools import wraps
from glob import glob

from wazuh import common
from wazuh.database import Connection
from wazuh.exception import WazuhError, WazuhInternalError


def _get_groups_resources(agent_id):
    """Obtain group resources based on agent_id (all the groups where the agent belongs)

    :param agent_id: Agent_id to search groups for
    :return: Group resources
    """
    db_global = glob(common.database_path_global)
    if not db_global:
        raise WazuhInternalError(1600)

    conn = Connection(db_global[0])
    if agent_id == '*':
        groups = ['agent:group:*']
        conn.execute("SELECT name FROM `group` WHERE id IN (SELECT DISTINCT id_group FROM belongs)")
    else:
        groups = ['agent:id:*', 'agent:group:*']
        conn.execute("SELECT name FROM `group` WHERE id IN (SELECT id_group FROM belongs WHERE id_agent = :agent_id)",
                     {'agent_id': int(agent_id)})
    result = conn.fetch_all()

    for group in result:
        groups.append('{0}:{1}'.format('agent:group', group[0]))

    return groups


def _get_required_permissions(actions: list = None, resources: str = None, **kwargs):
    """Obtain action:resource pairs exposed by the framework function

    :param actions: List of exposed actions
    :param resources: List of exposed resources
    :param kwargs: Function kwargs to look for dynamic resources
    :return: Dictionary with required actions as keys and a list of required resources as values
    """
    # We expose required resources for the request
    m = re.search(r'^(\w+\:\w+:)(\w+|\*|{(\w+)})$', resources)
    res_list = list()
    res_base = m.group(1)
    # If we find a '{' in the regex we obtain the dynamic resource/s
    if '{' in m.group(2):
        try:
            # Dynamic resources ids are found within the {}
            params = kwargs[m.group(3)]
            # We check if params is a list of resources or a single one in a string
            if isinstance(params, list):
                for param in params:
                    res_list.append("{0}{1}".format(res_base, param))
            else:
                res_list.append("{0}{1}".format(res_base, params))
        # KeyError occurs if required dynamic resources can't be found within request parameters
        except KeyError as e:
            raise WazuhInternalError(4014, extra_message=str(e))
    # If we don't find a regex match we obtain the static resource/s
    else:
        res_list.append(resources)

    # Create dict of required policies with action: list(resources) pairs
    req_permissions = dict()
    for action in actions:
        req_permissions[action] = res_list

    return req_permissions


def _match_permissions(req_permissions: dict = None, rbac: list = None):
    """Try to match function required permissions against user permissions to allow or deny execution

    :param req_permissions: Required permissions to allow function execution
    :param rbac: User permissions
    :return: Allow or deny
    """
    mode = rbac[0]
    user_permissions = rbac[1]
    # We run through all required permissions for the request
    for req_action, req_resources in req_permissions.items():
        for req_resource in req_resources:
            # allow_match is used to keep track when a required permission is matched by a policy with an allowed effect
            allow_match = False
            # We run through the user permissions to find a match with the required permissions
            for policy in user_permissions:
                # We find if action matches
                action_match = req_action in policy['actions']
                if action_match:
                    # We find resource name to add * if not already there
                    m = re.search(r'^(\w+\:\w+:)([\w\-\.\/]+|\*)$', req_resource)
                    res_match = False
                    # We find if resource matches
                    if m.group(1) == 'agent:id:':
                        reqs = [req_resource]
                        reqs.extend(_get_groups_resources(m.group(2)))
                        for req in reqs:
                            res_match = res_match or (req in policy['resources'])
                    elif m.group(2) != '*':
                        req_asterisk = '{0}{1}'.format(m.group(1), '*')
                        res_match = (req_resource in policy['resources']) or (req_asterisk in policy['resources'])
                    else:
                        res_match = req_resource in policy['resources']
                    # When any policy with a deny effect matches, we deny the request directly
                    if res_match and policy['effect'] == "deny":
                        return False
                    # When any policy with an allow effect matches, we set a match in allow_match and
                    # break out to continue with required permissions
                    elif res_match and policy['effect'] == "allow":
                        allow_match = True
                        break
            # If we are using white list mode and no match is found for the required permission we deny the request.
            if not allow_match and not mode:
                return False
    # If we don't find a deny match or we find an allow match for all policies in white list mode we allow the request
    return True


def expose_resources(actions: list = None, resources: str = None):
    """Decorator to apply user permissions on a Wazuh framework function based on exposed action:resource pairs.

    :param actions: List of actions exposed by the framework function
    :param resources: List of resources exposed by the framework function
    :return: Allow or deny framework function execution
    """
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            req_permissions = _get_required_permissions(actions=actions, resources=resources, **kwargs)
            allow = _match_permissions(req_permissions=req_permissions, rbac=kwargs['rbac'])
            if allow:
                del kwargs['rbac']
                return func(*args, **kwargs)
            else:
                raise WazuhError(4000)
        return wrapper
    return decorator
