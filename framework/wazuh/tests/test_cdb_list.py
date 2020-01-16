#!/usr/bin/env python
# Copyright (C) 2015-2020, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2
from functools import wraps
from unittest.mock import patch, MagicMock
import os
import pytest
import sys

DATA_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)), "data", "test_cdb_list")

with patch('wazuh.common.getgrnam'):
    with patch('wazuh.common.getpwnam'):
        with patch('wazuh.common.lists_path', DATA_PATH):
            sys.modules['wazuh.rbac.orm'] = MagicMock()
            sys.modules['api'] = MagicMock()
            import wazuh.rbac.decorators
            del sys.modules['wazuh.rbac.orm']
            del sys.modules['api']

            def RBAC_bypasser(**kwargs):
                def decorator(f):
                    @wraps(f)
                    def wrapper(*args, **kwargs):
                        return f(*args, **kwargs)
                    return wrapper
                return decorator
            wazuh.rbac.decorators.expose_resources = RBAC_bypasser
            sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
            from wazuh.cdb_list import get_lists, get_path_lists
            from wazuh import common
            from wazuh.results import AffectedItemsWazuhResult

RELATIVE_PATH = os.path.join("framework", "wazuh", "tests", "data", "test_cdb_list")
NAME_FILE_1 = "test_lists_1"
NAME_FILE_2 = "test_lists_2"
PATH_FILE_1 = os.path.join(RELATIVE_PATH, NAME_FILE_1)
PATH_FILE_2 = os.path.join(RELATIVE_PATH, NAME_FILE_2)
PATHS_FILES = [PATH_FILE_1, PATH_FILE_2]

RESULT_GET_LIST_FILE_1 = [{'items': [{'key': 'test-wazuh-w', 'value': 'write'},
                                     {'key': 'test-wazuh-r', 'value': 'read'},
                                     {'key': 'test-wazuh-a', 'value': 'attribute'},
                                     {'key': 'test-wazuh-x', 'value': 'execute'},
                                     {'key': 'test-wazuh-c', 'value': 'command'}
                                     ],
                           'path': PATH_FILE_1
                           }]
RESULT_GET_LIST_FILE_2 = [{'items': [{'key': 'test-ossec-w', 'value': 'write'},
                                     {'key': 'test-ossec-r', 'value': 'read'},
                                     {'key': 'test-ossec-x', 'value': 'execute'}
                                     ],
                           'path': PATH_FILE_2
                           }]
RESULT_GET_PATH_LIST_FILE_1 = [{'folder': RELATIVE_PATH, 'name': NAME_FILE_1, 'path': PATH_FILE_1}]
RESULT_GET_PATH_LIST_FILE_2 = [{'folder': RELATIVE_PATH, 'name': NAME_FILE_2, 'path': PATH_FILE_2}]

RESULTS_GET_LIST = RESULT_GET_LIST_FILE_1 + RESULT_GET_LIST_FILE_2
RESULTS_GET_PATH_LIST = RESULT_GET_PATH_LIST_FILE_1 + RESULT_GET_PATH_LIST_FILE_2


# Tests

@pytest.mark.parametrize("paths, expected_result", [
    ([PATH_FILE_1], RESULT_GET_LIST_FILE_1),
    (PATHS_FILES, RESULTS_GET_LIST)
])
def test_get_lists(paths, expected_result):
    """Test basic `get_list` functionality.

    This will obtain the content of some CDB lists using `get_list'' without any other parameter aside from `path`.

    Parameters
    ----------
    paths : list of str
        A list of CDB files to read, with their relative path.
    expected_result : list of dict
        The content of the CDB file or files read
    """
    result = get_lists(path=paths)
    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == len(paths)
    assert result.affected_items == expected_result


@pytest.mark.parametrize("limit", [1, 2])
def test_get_lists_limit(limit):
    """Test `get_lists` functionality when using the `limit` parameter.

    Parameters
    ----------
    limit : int
        Maximum number of items to be returned by `get_lists`
    """
    result = get_lists(path=PATHS_FILES, limit=limit)
    assert limit > 0
    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == limit
    assert result.affected_items == RESULTS_GET_LIST[:limit]


@pytest.mark.parametrize("offset", [0, 1])
def test_get_lists_offset(offset):
    """Test `get_lists` functionality when using the `offset` parameter.

    Parameters
    ----------
    offset : int
         Indicates the first item to return.
    """
    result = get_lists(path=PATHS_FILES, offset=offset)
    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == len(PATHS_FILES) - offset
    assert result.affected_items == RESULTS_GET_LIST[offset:]


@pytest.mark.parametrize("search_text, complementary_search, search_in_fields, paths, expected_result", [
    ("command", False, None, PATHS_FILES, RESULT_GET_LIST_FILE_1),
    ("test-ossec-w", False, None, PATHS_FILES, RESULT_GET_LIST_FILE_2),
    ("command", False, None, [PATH_FILE_2], []),
    ("command", False, None, PATHS_FILES, RESULT_GET_LIST_FILE_1),
    ("command", False, "items", [PATH_FILE_2], []),
    ("write", False, "items", PATHS_FILES, RESULTS_GET_LIST),
    ("test-wazuh-w", False, "items", PATHS_FILES, RESULT_GET_LIST_FILE_1),
    ("test-ossec-w", False, "items", PATHS_FILES, RESULT_GET_LIST_FILE_2),
    ("test-wazuh-w", False, "items", [PATH_FILE_2], []),
    ("command", True, None, PATHS_FILES, RESULT_GET_LIST_FILE_2),
    ("test-ossec-w", True, None, PATHS_FILES, RESULT_GET_LIST_FILE_1),
    ("command", True, None, [PATH_FILE_2], RESULT_GET_LIST_FILE_2),
    ("command", True, "items", PATHS_FILES, RESULT_GET_LIST_FILE_2),
    ("command", True, "items", [PATH_FILE_2], RESULT_GET_LIST_FILE_2),
    ("command", True, "items", [PATH_FILE_1], []),
    ("write", True, "items", PATHS_FILES, []),
    ("test-wazuh-w", True, "items", PATHS_FILES, RESULT_GET_LIST_FILE_2),
    ("test-ossec-w", True, "items", PATHS_FILES, RESULT_GET_LIST_FILE_1),
    ("test-wazuh-w", True, "items", [PATH_FILE_2], RESULT_GET_LIST_FILE_2),
])
def test_get_lists_search(search_text, complementary_search, search_in_fields, paths, expected_result):
    """Test `get_lists` functionality when using the `search` parameter.

    Parameters
    ----------
    search_text : str
        The text to search.
    complementary_search : bool
        If True, only results NOT containing `search_text` will be returned. If False, only results that contains
        `search_text` will be returned.
    search_in_fields : str
        Name of the field to search in for the `search_text`.
    paths : list of str
        A list of CDB files to read, with their relative path.
    expected_result : list of dict
        The content expected to be returned by `get_lists` when using the specified search parameters.
    """
    result = get_lists(path=paths, search_text=search_text, complementary_search=complementary_search,
                       search_in_fields=search_in_fields)
    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == len(expected_result)
    assert result.affected_items == expected_result


def test_get_lists_sort():
    """Test `get_lists` functionality when using the `sort` parameter."""
    result_a = get_lists(path=PATHS_FILES, sort_by=['path'], sort_ascending=True)
    result_b = get_lists(path=PATHS_FILES, sort_by=['path'], sort_ascending=False)

    assert isinstance(result_a, AffectedItemsWazuhResult)
    assert isinstance(result_b, AffectedItemsWazuhResult)
    assert result_a.affected_items != result_b.affected_items
    assert result_a.affected_items == RESULT_GET_LIST_FILE_1 + RESULT_GET_LIST_FILE_2
    assert result_b.affected_items == RESULT_GET_LIST_FILE_2 + RESULT_GET_LIST_FILE_1


def test_get_path_lists():
    """Test `get_path_lists` functionality without any other parameter aside from `path`.

    `get_path_lists` works different than `get_lists` as it will read every CDB file from the default path (mocked to
    `DATA_PATH`) and will remove from the result any file that is not in the `path` parameter provided.
    """
    common.reset_context_cache()
    result = get_path_lists(path=[PATH_FILE_1])

    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == len(RESULT_GET_PATH_LIST_FILE_1)
    assert result.affected_items == RESULT_GET_PATH_LIST_FILE_1


@pytest.mark.parametrize("limit", [1, 2])
def test_get_path_lists_limit(limit):
    """Test `get_path_lists` functionality when using the `limit` parameter.

    Parameters
    ----------
    limit : int
        Maximum number of items to be returned by `get_path_lists`
    """
    common.reset_context_cache()
    result = get_path_lists(path=PATHS_FILES, limit=limit)

    assert limit > 0
    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == limit
    assert result.affected_items == RESULTS_GET_PATH_LIST[:limit]


@pytest.mark.parametrize("offset", [0, 1])
def test_get_path_lists_offset(offset):
    """Test `get__path_lists` functionality when using the `offset` parameter.

    Parameters
    ----------
    offset : int
         Indicates the first item to return.
    """
    common.reset_context_cache()
    result = get_path_lists(path=PATHS_FILES, offset=offset)

    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == len(PATHS_FILES) - offset
    assert result.affected_items == RESULTS_GET_PATH_LIST[offset:]


@pytest.mark.parametrize("search_text, complementary_search, search_in_fields, paths, expected_result", [
    ("lists_1", False, None, PATHS_FILES, RESULT_GET_PATH_LIST_FILE_1),
    ("lists_2", False, None, PATHS_FILES, RESULT_GET_PATH_LIST_FILE_2),
    ("invalid", False, None, PATHS_FILES, []),
    ("lists_1", False, "path", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_1),
    ("lists_2", False, "path", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_2),
    ("invalid", False, "path", PATHS_FILES, []),
    ("lists_1", False, "name", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_1),
    ("lists_2", False, "name", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_2),
    ("invalid", False, "name", PATHS_FILES, []),
    ("test_cdb_list", False, "folder", PATHS_FILES, RESULTS_GET_PATH_LIST),

    ("lists_1", True, None, PATHS_FILES, RESULT_GET_PATH_LIST_FILE_2),
    ("lists_2", True, None, PATHS_FILES, RESULT_GET_PATH_LIST_FILE_1),
    ("invalid", True, None, PATHS_FILES, RESULTS_GET_PATH_LIST),
    ("lists_1", True, "path", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_2),
    ("lists_2", True, "path", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_1),
    ("invalid", True, "path", PATHS_FILES, RESULTS_GET_PATH_LIST),
    ("lists_1", True, "name", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_2),
    ("lists_2", True, "name", PATHS_FILES, RESULT_GET_PATH_LIST_FILE_1),
    ("invalid", True, "name", PATHS_FILES, RESULTS_GET_PATH_LIST),
    ("test_cdb_list", True, "folder", PATHS_FILES, [])
])
def test_get_path_lists_search(search_text, complementary_search, search_in_fields, paths, expected_result):
    """Test `get_path_lists` functionality when using the `search` parameter.

    Parameters
    ----------
    search_text : str
        The text to search.
    complementary_search : bool
        If True, only results NOT containing `search_text` will be returned. If False, only results that contains
        `search_text` will be returned.
    search_in_fields : str
        Name of the field to search in for the `search_text`.
    paths : list of str
        A list of CDB files to read, with their relative path.
    expected_result : list of dict
        The content expected to be returned by `get_lists` when using the specified search parameters.
    """
    common.reset_context_cache()
    result = get_path_lists(path=paths, search_text=search_text, complementary_search=complementary_search,
                            search_in_fields=search_in_fields)
    assert isinstance(result, AffectedItemsWazuhResult)
    assert result.total_affected_items == len(expected_result)
    assert result.affected_items == expected_result


def test_get_path_lists_sort():
    """Test `get_path_lists` functionality when using the `sort` parameter."""
    result_a = get_path_lists(path=PATHS_FILES, sort_by=['name'], sort_ascending=True)
    result_b = get_path_lists(path=PATHS_FILES, sort_by=['name'], sort_ascending=False)

    assert isinstance(result_a, AffectedItemsWazuhResult)
    assert isinstance(result_b, AffectedItemsWazuhResult)
    assert result_a.affected_items != result_b.affected_items
    assert result_a.affected_items == RESULT_GET_PATH_LIST_FILE_1 + RESULT_GET_PATH_LIST_FILE_2
    assert result_b.affected_items == RESULT_GET_PATH_LIST_FILE_2 + RESULT_GET_PATH_LIST_FILE_1
