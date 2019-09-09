# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import re
from os import listdir
from os.path import isfile, isdir, join

from wazuh import common
from wazuh.exception import WazuhError
from wazuh.utils import process_array

_regex_path = r'^(etc/lists/)[\w\.\-/]+$'
_pattern_path = re.compile(_regex_path)


def _get_relative_path(path):
    """Get relative path

    :param path: Original path
    :return: Relative path (from Wazuh base directory)
    """
    return path.replace(common.ossec_path, '')[1:]


def _check_path(path):
    """Check if path is well formed (without './' or '../')

    :param path: Path to check
    :return: Result of check the path (boolean)
    """
    if './' in path or '../' in path or not _pattern_path.match(path):
        raise WazuhError(1801)


def _iterate_lists(absolute_path, only_names=False):
    """Get the content of all CDB lists

    :param absolute_path: Full path of directory to get CDB lists
    :param only_names: If this parameter is true, only the name of all lists will be showed
    :return: List with all CDB lists
    """
    output = []
    dir_content = listdir(absolute_path)

    # for skipping .swp files
    regex_swp = r'^\.{1}[\w\-/]+(.swp){1}$'
    pattern = re.compile(regex_swp)

    for name in dir_content:
        new_absolute_path = join(absolute_path, name)
        new_relative_path = _get_relative_path(new_absolute_path)
        # '.cdb' and '.swp' files are skipped
        if (isfile(new_absolute_path)) and ('.cdb' not in name) and ('~' not in name) and not pattern.search(name):
            if only_names:
                relative_path = _get_relative_path(absolute_path)
                output.append({'path': '{0}/{1}'.format(relative_path, name), 'name': name, 'folder': relative_path})
            else:
                items = get_list_from_file(new_relative_path)
                output.append({'path': new_relative_path, 'items': items})
        elif isdir(new_absolute_path):
            if only_names:
                output += _iterate_lists(new_absolute_path, only_names=True)
            else:
                output += _iterate_lists(new_absolute_path)

    return output


def get_lists(path=None, offset=0, limit=common.database_limit, sort_by=None, sort_ascending=True, search_text=None,
              complementary_search=False, search_in_fields=None):
    """Get CDB lists

    :param path: Relative path of list file to get (if it is not specified, all lists will be returned)
    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort_by: Fields to sort the items by
    :param sort_ascending: Sort in ascending (true) or descending (false) order
    :param search_text: Text to search
    :param complementary_search: Find items without the text to search
    :param search_in_fields: Fields to search in
    :return: Dictionary: {'items': array of items, 'totalItems': Number of items (without applying the limit)}
    """
    lists = _iterate_lists(common.lists_path)

    if path is not None:
        # check if path is correct
        _check_path(path)
        for l in list(lists):
            if path != l['path']:
                lists.remove(l)
                continue

    return process_array(lists, search_text=search_text, search_in_fields=search_in_fields,
                         complementary_search=complementary_search, sort_by=sort_by, sort_ascending=sort_ascending,
                         allowed_sort_fields=['path'], offset=offset, limit=limit)


def get_list_from_file(file_path):
    """
    Get CDB list from file
    :param file_path: Relative path of list file to get
    :return: CDB list
    """
    file_path = join(common.ossec_path, file_path)
    output = []

    try:
        with open(file_path) as f:
            for line in f.read().splitlines():
                if 'TEMPLATE' in line:
                    continue
                else:
                    key, value = line.split(':')
                    output.append({'key': key, 'value': value})
    except OSError as e:
        if e.errno == 2:
            raise WazuhError(1802)
        elif e.errno == 13:
            raise WazuhError(1803)
        elif e.errno == 21:
            raise WazuhError(1804, extra_message="{0} {1}".format(join('WAZUH_HOME', file_path), "is a directory"))
        else:
            raise e
    except ValueError:
        raise WazuhError(1800, extra_message={'path': join('WAZUH_HOME', file_path)})

    return output


def get_list(file_path):
    """Get single CDB list from file

    :param file_path: Relative path of list file to get
    :return: CDB list
    """
    return {'items': get_list_from_file(file_path)}


def get_path_lists(offset=0, limit=common.database_limit, sort_by=None, sort_ascending=True, search_text=None,
                   complementary_search=False, search_in_fields=None):
    """Get paths of all CDB lists

    :param offset: First item to return.
    :param limit: Maximum number of items to return.
    :param sort_by: Fields to sort the items by
    :param sort_ascending: Sort in ascending (true) or descending (false) order
    :param search_text: Text to search
    :param complementary_search: Find items without the text to search
    :param search_in_fields: Fields to search in
    :return: Dictionary: {'items': array of items, 'totalItems': Number of items (without applying the limit)}
    """
    output = _iterate_lists(common.lists_path, only_names=True)

    return process_array(output, search_text=search_text, search_in_fields=search_in_fields,
                         complementary_search=complementary_search, sort_by=sort_by, sort_ascending=sort_ascending,
                         offset=offset, limit=limit)
