# Copyright (C) 2015, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

from os import path
import copy

# RETRIES CONFIGURATIONS
RETRY_ATTEMPTS_KEY = "max_attempts"
RETRY_MODE_CONFIG_KEY = "retry_mode"
RETRY_MODE_BOTO_KEY = "mode"
WAZUH_DEFAULT_RETRY_CONFIGURATION = {RETRY_ATTEMPTS_KEY: 10, RETRY_MODE_BOTO_KEY: 'standard'}


# AWS BUCKET CONFIGURATIONS
MAX_AWS_BUCKET_RECORD_RETENTION = 500
AWS_BUCKET_DB_DATE_FORMAT = "%Y%m%d"

# CLOUDTRAIL CONFIGURATIONS
AWS_CLOUDTRAIL_DYNAMIC_FIELDS = ['additionalEventData', 'responseElements', 'requestParameters']

# MSG HEADERS
WAZUH_AWS_MESSAGE_HEADER = "1:Wazuh-AWS:"

# MSG TEMPLATES
AWS_BUCKET_MSG_TEMPLATE = {'integration': 'aws',
                           'aws': {'log_info': {'aws_account_alias': '', 'log_file': '', 's3bucket': ''}}}
AWS_SERVICE_MSG_TEMPLATE = {'integration': 'aws', 'aws': ''}


# PATHS
DEFAULT_AWS_CONFIG_PATH = path.join(path.expanduser('~'), '.aws', 'config')
QUEUE_PATH = 'queue/sockets/queue'
WODLE_PATH = 'wodles/aws'
TEST_WAZUH_PATH = "/var/ossec"


# DATABASE VALUES
DEFAULT_AWS_SERVICE_DATABASE_NAME = "aws_services"
DEFAULT_AWS_SERVICE_TABLENAME = "aws_services"
DEFAULT_AWS_BUCKET_DATABASE_NAME = "s3_cloudtrail"
TEST_DATABASE = "test"

# DATABASE QUERIES
SQL_COUNT_ROWS = """SELECT count(*) FROM {table_name};"""

# REGIONS
ALL_REGIONS = (
    'af-south-1', 'ap-east-1', 'ap-northeast-1', 'ap-northeast-2', 'ap-northeast-3', 'ap-south-1', 'ap-south-2',
    'ap-southeast-1', 'ap-southeast-2', 'ap-southeast-3', 'ap-southeast-4', 'ca-central-1', 'eu-central-1',
    'eu-central-2', 'eu-north-1', 'eu-south-1', 'eu-south-2', 'eu-west-1', 'eu-west-2', 'eu-west-3', 'il-central-1',
    'me-central-1', 'me-south-1', 'sa-east-1', 'us-east-1', 'us-east-2', 'us-west-1', 'us-west-2'
)
DEFAULT_AWS_INTEGRATION_GOV_REGIONS = {'us-gov-east-1', 'us-gov-west-1'}
SERVICES_REQUIRING_REGION = {'inspector', 'cloudwatchlogs'}


# URLS
RETRY_CONFIGURATION_URL = 'https://documentation.wazuh.com/current/amazon/services/prerequisites/' \
                          'considerations.html#Connection-configuration-for-retries'
GUARDDUTY_URL = 'https://documentation.wazuh.com/current/amazon/services/supported-services/guardduty.html'
AWS_CREDENTIALS_URL = 'https://documentation.wazuh.com/current/amazon/services/prerequisites/credentials.html'
SECURITY_LAKE_IAM_ROLE_AUTHENTICATION_URL = 'https://documentation.wazuh.com/current/cloud-security/amazon/services/' \
                                        'supported-services/security-lake.html#configuring-an-iam-role'

# AWS INTEGRATION DEPRECATED VALUES
DEPRECATED_AWS_INTEGRATION_TABLES = {'log_progress', 'trail_progress'}

# DEPRECATED MESSAGES
GUARDDUTY_DEPRECATED_MESSAGE = 'The functionality to process GuardDuty logs stored in S3 via Kinesis was deprecated ' \
                               'in {release}. Consider configuring GuardDuty to store its findings directly in an S3 ' \
                               'bucket instead. Check {url} for more information.'

AWS_AUTH_DEPRECATED_MESSAGE = 'The {name} authentication parameter was deprecated in {release}. ' \
                     'Please use another authentication method instead. Check {url} for more information.'


# ERROR CODES
THROTTLING_EXCEPTION_ERROR_CODE = "ThrottlingException"
UNKNOWN_ERROR_CODE = 1
INVALID_CREDENTIALS_ERROR_CODE = 3
METADATA_ERROR_CODE = 5
UNABLE_TO_CREATE_DB = 6
UNEXPECTED_ERROR_WORKING_WITH_S3 = 7
DECOMPRESS_FILE_ERROR_CODE = 8
PARSE_FILE_ERROR_CODE = 9
UNABLE_TO_CONNECT_SOCKET_ERROR_CODE = 11
INVALID_TYPE_ERROR_CODE = 12
SENDING_MESSAGE_SOCKET_ERROR_CODE = 13
EMPTY_BUCKET_ERROR_CODE = 14
INVALID_ENDPOINT_ERROR_CODE = 15
THROTTLING_ERROR_CODE = 16
INVALID_KEY_FORMAT_ERROR_CODE = 17
INVALID_PREFIX_ERROR_CODE = 18
INVALID_REQUEST_TIME_ERROR_CODE = 19
UNABLE_TO_FETCH_DELETE_FROM_QUEUE = 21
INVALID_REGION_ERROR_CODE = 22


# ERROR MESSAGES
UNKNOWN_ERROR_MESSAGE = "Unexpected error: '{error}'."
INVALID_CREDENTIALS_ERROR_MESSAGE = "Invalid credentials to access S3 Bucket"
INVALID_REQUEST_TIME_ERROR_MESSAGE = "The server datetime and datetime of the AWS environment differ"
THROTTLING_EXCEPTION_ERROR_MESSAGE = "The '{name}' request was denied due to request throttling. " \
                                     "If the problem persists check the following link to learn how to use " \
                                     f"the Retry configuration to avoid it: '{RETRY_CONFIGURATION_URL}'"

# TESTS VALUES
TEST_HARDCODED_WAZUH_VERSION = "4.5.0"
TEST_TABLE_NAME = "cloudtrail"
TEST_SERVICE_NAME = "s3"
TEST_ACCESS_KEY = "test_access_key"
TEST_SECRET_KEY = "test_secret_key"
TEST_AWS_PROFILE = "test_aws_profile"
TEST_IAM_ROLE_ARN = "arn:aws:iam::123455678912:role/Role"
TEST_IAM_ROLE_DURATION = '3600'
TEST_ACCOUNT_ID = "123456789123"
TEST_ACCOUNT_ALIAS = "test_account_alias"
TEST_ORGANIZATION_ID = "test_organization_id"
TEST_TOKEN = 'test_token'
TEST_CREATION_DATE = "2022-01-01"
TEST_BUCKET = "test-bucket"
TEST_SERVICE = "test-service"
TEST_SQS_NAME = "test-sqs"
TEST_PREFIX = "test_prefix"
TEST_SUFFIX = "test_suffix"
TEST_REGION = "us-east-1"
TEST_DISCARD_FIELD = "test_field"
TEST_DISCARD_REGEX = "test_regex"
TEST_ONLY_LOGS_AFTER = "20220101"
TEST_ONLY_LOGS_AFTER_WITH_FORMAT = "2022-01-01 00:00:00.0"
TEST_LOG_KEY = "123456789_CloudTrail-us-east-1_20190401T00015Z_aaaa.json.gz"
TEST_FULL_PREFIX = "base/account_id/service/region/"
TEST_EXTERNAL_ID = "external-id-Sec-Lake"

TEST_SERVICE_ENDPOINT = 'test_service_endpoint'
TEST_STS_ENDPOINT = "test_sts_endpoint"

TEST_LOG_FULL_PATH_CLOUDTRAIL_1 = 'AWSLogs/123456789/CloudTrail/us-east-1/2019/04/01/123456789_CloudTrail-us-east-1_' \
                                  '20190401T0030Z_aaaa.json.gz'
TEST_LOG_FULL_PATH_CLOUDTRAIL_2 = 'AWSLogs/123456789/CloudTrail/us-east-1/2019/04/01/123456789_CloudTrail-us-east-1_' \
                                  '20190401T00015Z_aaaa.json.gz'
TEST_LOG_FULL_PATH_CUSTOM_1 = 'custom/2019/04/15/07/firehose_custom-1-2019-04-15-09-16-03.zip'
TEST_LOG_FULL_PATH_CUSTOM_2 = 'custom/2019/04/15/07/firehose_custom-1-2019-04-15-13-19-03.zip'
TEST_LOG_FULL_PATH_CONFIG_1 = 'AWSLogs/123456789/Config/us-east-1/2019/4/15/ConfigHistory/123456789_Config_us-east-1_' \
                              'ConfigHistory_20190415T020500Z.json.gz'
TEST_MESSAGE = "test_message"


LIST_OBJECT_V2 = {'CommonPrefixes': [{'Prefix': f'AWSLogs/{TEST_REGION}/'},
                                     {'Prefix': f'AWSLogs/prefix/{TEST_REGION}/'}]}
LIST_OBJECT_V2_NO_PREFIXES = {'Contents': [{
    'Key': '',
    'OtherKey': 'value'}],
    'IsTruncated': False
}
LIST_OBJECT_V2_TRUNCATED = copy.deepcopy(LIST_OBJECT_V2_NO_PREFIXES)
LIST_OBJECT_V2_TRUNCATED.update({'IsTruncated': True, 'NextContinuationToken': 'Token'})
