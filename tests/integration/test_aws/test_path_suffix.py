"""
Copyright (C) 2015-2023, Wazuh Inc.
Created by Wazuh, Inc. <info@wazuh.com>.
This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

This module will contains all cases for the path suffix test suite
"""

import pytest

# qa-integration-framework imports
from wazuh_testing import session_parameters
from wazuh_testing.modules.aws import event_monitor, local_internal_options  # noqa: F401
from wazuh_testing.modules.aws.db_utils import (
    get_s3_db_row,
    s3_db_exists,
    table_exists_or_has_values,
)

# Local module imports
from .utils import ERROR_MESSAGES, TIMEOUTS
from conftest import TestConfigurator

pytestmark = [pytest.mark.server]

# Set test configurator for the module
configurator = TestConfigurator(module='path_suffix_test_module')

# ---------------------------------------------------- TEST_PATH -------------------------------------------------------
# Configure T1 test
configurator.configure_test(configuration_file='configuration_path_suffix.yaml',
                            cases_file='cases_path_suffix.yaml')


@pytest.mark.tier(level=0)
@pytest.mark.parametrize('configuration, metadata',
                         zip(configurator.test_configuration_template, configurator.metadata),
                         ids=configurator.cases_ids)
def test_path_suffix(
    configuration, metadata, load_wazuh_basic_configuration, set_wazuh_configuration, clean_s3_cloudtrail_db,
    configure_local_internal_options_function, truncate_monitored_files, restart_wazuh_function, file_monitoring
):
    """
    description: Only logs within a path_suffix are processed.
    test_phases:
        - setup:
            - Load Wazuh light configuration.
            - Apply ossec.conf configuration changes according to the configuration template and use case.
            - Apply custom settings in local_internal_options.conf.
            - Truncate wazuh logs.
            - Restart wazuh-manager service to apply configuration changes.
        - test:
            - Check in the ossec.log that a line has appeared calling the module with correct parameters.
            - If a path_suffix that does not exist was specified, make sure that a message is displayed in the ossec.log
              warning the user.
            - Check the command was called with the correct parameters.
            - Check the database was created and updated accordingly.
        - teardown:
            - Truncate wazuh logs.
            - Restore initial configuration, both ossec.conf and local_internal_options.conf.
            - Delete the uploaded file.
    wazuh_min_version: 4.6.0
    parameters:
        - configuration:
            type: dict
            brief: Get configurations from the module.
        - metadata:
            type: dict
            brief: Get metadata from the module.
        - load_wazuh_basic_configuration:
            type: fixture
            brief: Load basic wazuh configuration.
        - set_wazuh_configuration:
            type: fixture
            brief: Apply changes to the ossec.conf configuration.
        - clean_s3_cloudtrail_db:
            type: fixture
            brief: Delete the DB file before and after the test execution
        - configure_local_internal_options_function:
            type: fixture
            brief: Apply changes to the local_internal_options.conf configuration.
        - truncate_monitored_files:
            type: fixture
            brief: Truncate wazuh logs.
        - restart_wazuh_daemon_function:
            type: fixture
            brief: Restart the wazuh service.
        - file_monitoring:
            type: fixture
            brief: Handle the monitoring of a specified file.
    assertions:
        - Check in the log that the module was called with correct parameters.
        - Check the expected number of events were forwarded to analysisd.
        - Check the database was created and updated accordingly, using the correct path for each entry.
    input_description:
        - The `configuration_path_suffix` file provides the module configuration for this test.
        - The `cases_path_suffix` file provides the test cases.
    """
    bucket_name = metadata['bucket_name']
    bucket_type = metadata['bucket_type']
    only_logs_after = metadata['only_logs_after']
    path_suffix = metadata['path_suffix']
    expected_results = metadata['expected_results']
    pattern = (
        fr".*No logs found in 'AWSLogs/{path_suffix}/'. "
        fr"Check the provided prefix and the location of the logs for the bucket type '{bucket_type}'*"
    )

    parameters = [
        'wodles/aws/aws-s3',
        '--bucket', bucket_name,
        '--aws_profile', 'qa',
        '--trail_suffix', path_suffix,
        '--only_logs_after', only_logs_after,
        '--type', bucket_type,
        '--debug', '2'
    ]

    # Check AWS module started
    log_monitor.start(
        timeout=session_parameters.default_timeout,
        callback=event_monitor.callback_detect_aws_module_start
    )

    assert log_monitor.callback_result is not None, ERROR_MESSAGES['failed_start']

    # Check command was called correctly
    log_monitor.start(
        timeout=session_parameters.default_timeout,
        callback=event_monitor.callback_detect_aws_module_called(parameters)
    )

    assert log_monitor.callback_result is not None, ERROR_MESSAGES['incorrect_parameters']

    if expected_results:
        log_monitor.start(
            timeout=TIMEOUTS[20],
            callback=event_monitor.callback_detect_event_processed,
        )
        assert log_monitor.callback_result is not None, ERROR_MESSAGES['incorrect_event_number']

    else:
        log_monitor.start(
            timeout=TIMEOUTS[10],
            callback=event_monitor.make_aws_callback(pattern),
        )

        assert log_monitor.callback_result is not None, ERROR_MESSAGES['incorrect_empty_path_suffix_message']

    assert s3_db_exists()

    if expected_results:
        data = get_s3_db_row(table_name=bucket_type)
        assert f"{bucket_name}/{path_suffix}/" == data.bucket_path
        assert data.log_key.startswith(f"AWSLogs/{path_suffix}/")
    else:
        assert not table_exists_or_has_values(table_name=bucket_type)

    # Detect any ERROR message
    log_monitor.start(
        timeout=session_parameters.default_timeout,
        callback=event_monitor.callback_detect_all_aws_err
    )

    assert log_monitor.callback_result is None, ERROR_MESSAGES['error_found']
