# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import os
import sys
from functools import wraps
from unittest.mock import patch, MagicMock, mock_open

import pytest

from wazuh.exception import WazuhException

with patch('wazuh.common.ossec_uid'):
    with patch('wazuh.common.ossec_gid'):
        sys.modules['wazuh.rbac.orm'] = MagicMock()
        sys.modules['api'] = MagicMock()
        import wazuh.rbac.decorators
        del sys.modules['wazuh.rbac.orm']
        del sys.modules['api']

        def RBAC_bypasser(**kwargs_decorator):
            def decorator(f):
                @wraps(f)
                def wrapper(*args, **kwargs):
                    return f(*args, **kwargs)
                return wrapper
            return decorator
        wazuh.rbac.decorators.expose_resources = RBAC_bypasser
        from wazuh.stats import *

with patch('wazuh.common.ossec_uid'):
    with patch('wazuh.common.ossec_gid'):
        from wazuh.stats import *


@pytest.mark.parametrize('year, month, day, data_list', [
    (2019, "Aug", 13, ['15-571-3-2', '15--107--1483--1257--0']),
    (2019, "Aug", 13, ['15-571-3-2']),
    (2019, "Aug", 13, ['15--107--1483--1257--0']),
    (2019, "Aug", 13, ['15'])
])
@patch('wazuh.stats.common.stats_path', new='/stats')
def test_totals(year, month, day, data_list):
    """Verify totals() function works returns and correct data

    Checks data type of returned data. Then makes sure that data returned fit
    with the expected.

    Parameters
    ----------
    year : str
        Name of 'year' directory
    month : str
        Name of 'month' directory
    day : str
        Name of 'day' file
    data_list : list
        Data to use instead of the original files.
    """
    with patch('wazuh.stats.open', return_value=data_list):
        response = totals(year, month, day)

        assert isinstance(response, WazuhResult), f'The result is not WazuhResult type'

        dict_res = response.to_dict()
        if dict_res['result']['data']:
            for line in data_list:
                data = line.split('-')
                if len(data) == 4:
                    assert int(data[1]) == dict_res['result']['data'][0]['alerts'][0]['sigid'], f'Data do not match'
                    assert int(data[2]) == dict_res['result']['data'][0]['alerts'][0]['level'], f'Data do not match'
                    assert int(data[3]) == dict_res['result']['data'][0]['alerts'][0]['times'], f'Data do not match'
                else:
                    data = line.split('--')
                    if len(data) == 5:
                        assert int(data[0]) == dict_res['result']['data'][0]['hour'], f'Data do not match'
                        assert int(data[1]) == dict_res['result']['data'][0]['totalAlerts'], f'Data do not match'
                        assert int(data[2]) == dict_res['result']['data'][0]['events'], f'Data do not match'
                        assert int(data[3]) == dict_res['result']['data'][0]['syscheck'], f'Data do not match'
                        assert int(data[4]) == dict_res['result']['data'][0]['firewall'], f'Data do not match'


@pytest.mark.parametrize('year, month, day, expected_exception', [
    (-1, "Aug", 0, 1307),
    (1, "Aug", 32, 1307),
    ("First", "Aug", 13, 1307),
    (2019, "Test", 13, 1307),
    (2019, 13, 13, 1307),
    (2019, 12, 13, 1307)
])
@patch('wazuh.stats.MONTHS', new=['One', 'Two'])
def test_totals_ko_date(year, month, day, expected_exception):
    """Tests totals function exception with date problems works

    Parameters
    ----------
    year : str
        Name of 'year' directory
    month : str
        Name of 'month' directory
    day : str
        Name of 'day' file
    expected_exception : int
        Expected exception code for given dates
    """
    with pytest.raises(WazuhException, match=f'.* {expected_exception} .*'):
        totals(year, month, day)


def test_totals_ko_data():
    """Tests totals function exception with data problems works"""
    with patch('wazuh.stats.open', side_effect=IOError):
        with pytest.raises(WazuhException, match=".* 1308 .*"):
            totals(1996, "Aug", 13, today=True)
        with pytest.raises(WazuhException, match=".* 1310 .*"):
            totals(1996, "Aug", 13)

    with patch('wazuh.stats.open', return_value=['15-571-3-2', '15--107--1483']):
        with pytest.raises(WazuhException, match=".* 1309 .*"):
            totals(1996, "Aug", 13)


@pytest.mark.parametrize('effect', [
    None,
    IOError,
])
def test_hourly(effect):
    """Tests hourly() function works and returns correct data

    Parameters
    ----------
    effect : exception
        Exception expected when opening stats file
    """
    with patch('wazuh.stats.open', side_effect=effect):
        response = hourly()
        assert isinstance(response, WazuhResult), f'The result is not WazuhResult type'


@patch('wazuh.common.stats_path', new='wazuh/tests/data/stats')
def test_hourly_data():
    """Makes sure that data returned by hourly() fit with the expected."""
    response = hourly()

    assert 24 == response.to_dict()['result']['interactions'], f'Data do not match'
    for hour in range(24):
        assert hour in response.to_dict()['result']['averages'], f'Data do not match'


@pytest.mark.parametrize('effect', [
    None,
    IOError
])
def test_weekly(effect):
    """Tests weekly() function works and returns correct data

    Parameters
    ----------
    effect : exception
        Exception expected when opening stats file
    """
    with patch('wazuh.stats.open', side_effect=effect):
        response = weekly()
        assert isinstance(response, WazuhResult), f'The result is not WazuhResult type'


@patch('wazuh.common.stats_path', new='wazuh/tests/data/stats')
def test_weekly_data():
    """Makes sure that data returned by weekly() fit with the expected."""
    response = weekly()

    assert 0 == response.to_dict()['result']['Sun']['interactions'], f'Data do not match'
    for day in DAYS:
        assert day in response.to_dict()['result'].keys(), f'Data do not match'
    for hour in range(24):
        assert hour in response.to_dict()['result']['Sun']['hours'], f'Data do not match'


@patch('wazuh.stats.open')
@patch('wazuh.stats.configparser.RawConfigParser.read_file')
@patch('wazuh.stats.configparser.RawConfigParser.items', return_value={'hour':"'5'"})
def test_get_daemons_stats(mock_items, mock_read, mock_open):
    """Tests get_daemons_stats function works"""
    response = get_daemons_stats('filename')

    assert isinstance(response, dict), f'The result is not dict type'
    mock_open.assert_called_once_with('filename', 'r')


@patch('wazuh.stats.configparser.RawConfigParser.read_file')
def test_get_daemons_stats_ko(mock_readfp):
    """Tests get_daemons_stats function exceptions works"""

    with patch('wazuh.stats.open', side_effect=Exception):
        with pytest.raises(WazuhException, match=".* 1308 .*"):
            get_daemons_stats('filename')

    with patch('wazuh.stats.open'):
        with patch('wazuh.stats.configparser.RawConfigParser.items', return_value={'hour':5}):
            response = get_daemons_stats('filename')

            assert isinstance(response, WazuhException), f'The result is not WazuhResult type'
            assert response.code == 1104, f'Response code is not the same'
