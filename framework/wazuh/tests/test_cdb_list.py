#!/usr/bin/env python
# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import sys
from unittest import TestCase

from wazuh import WazuhException
from wazuh.cdb_list import get_lists


class TestCDBList(TestCase):

    def test_get_lists(self):
        result = get_lists()
        self.assertIsInstance(result, dict)
        self.assertIsInstance(result['totalItems'], int)
        self.assertIsInstance(result['items'], list)

    def test_get_lists_path(self):
        result = get_lists(path='etc/lists/audit-keys')
        self.assertEqual(len(result['items']), 1)

        with self.assertRaises(WazuhException) as cm:
            get_lists(path='wrong_path')   
            
        self.assertEqual(cm.exception.code, 1801)

    def test_get_lists_offset(self):
        result_a = get_lists(offset=0)
        result_b = get_lists(offset=1)
        self.assertNotEqual(result_a, result_b)

    def test_get_lists_search(self):
        # search parameter is a dictionary with values value and negation
        result = get_lists(search={'value': 'audit-keys', 'negation': 0})
        self.assertEqual(len(result['items']), 1)

        result = get_lists(search={'value': 'AAABBBCCC', 'negation': 0})
        self.assertEqual(len(result['items']), 0)

    def test_get_lists_sort(self):
        # sort parameter is a dictionary with values fields and order
        result_a = get_lists(sort={'fields': ['path'], 'order': 'asc'})
        result_b = get_lists(sort={'fields': ['path'], 'order': 'desc'})
        self.assertNotEqual(result_a, result_b)
