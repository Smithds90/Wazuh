# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import os
from enum import Enum
from glob import glob

from wazuh import common
from wazuh.exception import WazuhError
from wazuh.utils import load_wazuh_xml


class Status(Enum):
    S_ENABLED = 'enabled'
    S_DISABLED = 'disabled'
    S_ALL = 'all'
    SORT_FIELDS = ['file', 'path', 'description', 'id', 'level', 'status']


def add_detail(detail, value, details):
    """Add a rule detail (i.e. category, noalert, etc.).

    :param detail: Detail name.
    :param value: Detail value.
    :param details: Details dict.
    """
    if detail in details:
        # If it was an element, we create a list.
        if type(details[detail]) is not list:
            element = details[detail]
            details[detail] = [element]

        details[detail].append(value)
    else:
        details[detail] = value


def add_unique_element(src_list, element):
    new_list = list()
    new_list.extend(element) if type(element) in [list, tuple] else new_list.append(element)

    for item in new_list:
        if item is not None and item != '':
            i = item.strip()
            if i not in src_list:
                src_list.append(i)


def check_status(status):
    if status is None:
        return Status.S_ALL.value
    elif status in [Status.S_ALL.value, Status.S_ENABLED.value, Status.S_DISABLED.value]:
        return status
    else:
        raise WazuhError(1202)


def set_groups(groups, general_groups, rule):
    pci_groups, gpg13_groups, gdpr_groups, hipaa_groups, nist_800_53_groups, ossec_groups = (list() for i in range(6))
    requirements = {'pci_dss_': ('pci', pci_groups, 8), 'gpg13_': ('gpg13', gpg13_groups, 6),
                    'gdpr_': ('gdpr', gdpr_groups, 5), 'hipaa_': ('hipaa', hipaa_groups, 6),
                    'nist_800_53_': ('nist_800_53', nist_800_53_groups, 12), 'groups': ('groups', ossec_groups)}
    groups.extend(general_groups)
    for g in groups:
        for key, value in requirements.items():
            if key in g:
                value[1].append(g.strip()[value[2]:])
            else:
                requirements['groups'][1].append(g)

    for key, value in requirements.items():
        add_unique_element(rule[value[0]], value[1])


def load_rules_from_file(rule_file, rule_path, rule_status):
    try:
        rules = list()
        root = load_wazuh_xml(os.path.join(common.ossec_path, rule_path, rule_file))

        for xml_group in list(root):
            if xml_group.tag.lower() == "group":
                general_groups = xml_group.attrib['name'].split(',')
                for xml_rule in list(xml_group):
                    # New rule
                    if xml_rule.tag.lower() == "rule":
                        groups = list()
                        rule = {'file': rule_file, 'path': rule_path, 'id': int(xml_rule.attrib['id']),
                                'level': int(xml_rule.attrib['level']), 'status': rule_status, 'details': dict(),
                                'pci': list(), 'gpg13': list(), 'gdpr': list(), 'hipaa': list(), 'nist_800_53': list(),
                                'groups': list(), 'description': ''}
                        for k in xml_rule.attrib:
                            if k != 'id' and k != 'level':
                                rule['details'][k] = xml_rule.attrib[k]

                        for xml_rule_tags in list(xml_rule):
                            tag = xml_rule_tags.tag.lower()
                            value = xml_rule_tags.text
                            if value is None:
                                value = ''
                            if tag == "group":
                                groups.extend(value.split(","))
                            elif tag == "description":
                                rule['description'] += value
                            elif tag == "field":
                                add_detail(xml_rule_tags.attrib['name'], value, rule['details'])
                            elif tag in ("list", "info"):
                                list_detail = {'name': value}
                                for attrib, attrib_value in xml_rule_tags.attrib.items():
                                    list_detail[attrib] = attrib_value
                                add_detail(tag, list_detail, rule['details'])
                            # show rule variables
                            elif tag in {'regex', 'match', 'user', 'id'} and value != '' and value[0] == "$":
                                for variable in filter(lambda x: x.get('name') == value[1:], root.findall('var')):
                                    add_detail(tag, variable.text, rule['details'])
                            else:
                                add_detail(tag, value, rule['details'])

                        # Set groups
                        set_groups(groups=groups, general_groups=general_groups, rule=rule)
                        rules.append(rule)
    except OSError as e:
        if e.errno == 2:
            raise WazuhError(1201)
        elif e.errno == 13:
            raise WazuhError(1207)
        else:
            raise e

    return rules


def _remove_files(tmp_data, parameters):
    data = list(tmp_data)
    for d in tmp_data:
        for key, value in parameters.items():
            if key == 'status':
                value and value != Status.S_ALL.value and value != d[key] and data.remove(d)
            elif value and value != d[key]:
                data.remove(d)

    return data


def _create_rule_dir_dict(ruleset_conf, tag, exclude_filenames, data):
    items = ruleset_conf[tag] if type(ruleset_conf[tag]) is list else [ruleset_conf[tag]]
    for item_dir in items:
        all_rules = f"{common.ossec_path}/{item_dir}/*.xml"
        for item in glob(all_rules):
            item_name = os.path.basename(item)
            item_dir = os.path.relpath(os.path.dirname(item), start=common.ossec_path)
            item_status = Status.S_DISABLED.value if item_name in exclude_filenames else Status.S_ENABLED.value
            data.append({'file': item_name, 'path': item_dir, 'status': item_status})


def _create_rule_dict(ruleset_conf, tag, exclude_filenames, data):
    item_status = Status.S_DISABLED.value if tag == 'rule_exclude' else Status.S_ENABLED.value
    items = ruleset_conf[tag] if type(ruleset_conf[tag]) is list else [ruleset_conf[tag]]
    for item in items:
        item_name = os.path.basename(item)
        full_dir = os.path.dirname(item)
        item_dir = os.path.relpath(full_dir if full_dir else common.ruleset_rules_path, start=common.ossec_path)
        exclude_filenames.append(item_name) if tag == 'rule_exclude' else \
            data.append({'file': item_name, 'path': item_dir, 'status': item_status})


def format_rule_file(ruleset_conf, parameters):
    tmp_data, exclude_filenames = list(), list()
    tags = ['rule_include', 'rule_exclude', 'rule_dir']
    for tag in tags:
        if tag in ruleset_conf:
            _create_rule_dir_dict(ruleset_conf, tag, exclude_filenames, tmp_data) if tag == 'rule_dir' else\
                _create_rule_dict(ruleset_conf, tag, exclude_filenames, tmp_data)

    return _remove_files(tmp_data, parameters)
