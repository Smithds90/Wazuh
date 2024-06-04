# Copyright (C) 2015, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import json
import sys
from os import path
from typing import List

# Local imports
sys.path.insert(0, path.dirname(path.dirname(path.abspath(__file__))))
from aws_tools import  aws_logger


# Classes
class AWSQueueMessageProcessor:
    """Class in charge of processing the messages retrieved from an AWS SQS queue."""

    def extract_message_info(self, sqs_messages: List[dict]) -> List[dict]:
        messages = []
        for mesg in sqs_messages:
            body = mesg['Body']
            msg_handle = mesg["ReceiptHandle"]
            message = json.loads(body)

            aws_logger.debug(f'The message is: {message}')

            message_information = self.parse_message(message)
            messages.append({**message_information, "handle": msg_handle})
        return messages

    def parse_message(self, message: dict) -> dict:
        raise NotImplementedError


class AWSS3MessageProcessor(AWSQueueMessageProcessor):
    def parse_message(self, message: dict) -> dict:
        try:
            log_path = message["Records"][0]["s3"]["object"]["key"]
            bucket_path = message["Records"][0]["s3"]["bucket"]["name"]

            return {"route": {"log_path": log_path, "bucket_path": bucket_path}}
        except KeyError:
            return {'raw_message': message}


class AWSSSecLakeMessageProcessor(AWSQueueMessageProcessor):
    def parse_message(self, message: dict) -> dict:
        try:
            log_path = message["detail"]["object"]["key"]
            bucket_path = message["detail"]["bucket"]["name"]
            return {"route": {"log_path": log_path, "bucket_path": bucket_path}}
        except KeyError:
            return {'raw_message': message}
