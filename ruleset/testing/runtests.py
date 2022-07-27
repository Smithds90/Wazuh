#!/usr/bin/env python
# Copyright (C) 2015-2021, Wazuh Inc.
#
# This program is a free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public
# License (version 2) as published by the FSF - Free Software
# Foundation

import configparser
import subprocess
import os
import sys
from collections import OrderedDict
import shutil
import argparse
import re
import signal
import xml.etree.ElementTree as ET

from coverage import getRuleIds
from coverage import getParentDecoderNames

rules_test_fname_pattern = re.compile('^test_(.*?)_rules.xml$')
decoders_test_fname_pattern = re.compile('^test_(.*?)_decoders.xml$')

class MultiOrderedDict(OrderedDict):
    def __setitem__(self, key, value):
        if isinstance(value, list) and key in self:
            self[key].extend(value)
        else:
            super(MultiOrderedDict, self).__setitem__(key, value)


def getWazuhInfo(wazuh_home):
    wazuh_control = os.path.join(wazuh_home, "bin", "wazuh-control")
    try:
        proc = subprocess.Popen([wazuh_control, "info"], stdout=subprocess.PIPE)
        (stdout, stderr) = proc.communicate()
    except:
        print("Seems like there is no Wazuh installation.")
        return None
    return True

def provisionDR():
    base_dir = os.path.dirname(os.path.realpath(__file__))
    rules_dir = os.path.join(base_dir, "ruleset")
    decoders_dir = os.path.join(base_dir, "ruleset")

    for file in os.listdir(rules_dir):
        file_fullpath = os.path.join(rules_dir, file)
        if os.path.isfile(file_fullpath) and re.match(r'^test_(.*?)_rules.xml$',file):
            shutil.copy2(file_fullpath , args.wazuh_home + "/etc/rules")

    for file in os.listdir(decoders_dir):
        file_fullpath = os.path.join(decoders_dir, file)
        if os.path.isfile(file_fullpath) and re.match(r'^test_(.*?)_decoders.xml$',file):
            shutil.copy2(file_fullpath , args.wazuh_home + "/etc/decoders")

def cleanDR():
    rules_dir = args.wazuh_home + "/etc/rules"
    decoders_dir = args.wazuh_home + "/etc/decoders"

    for file in os.listdir(rules_dir):
        file_fullpath = os.path.join(rules_dir, file)
        if os.path.isfile(file_fullpath) and re.match(r'^test_(.*?)_rules.xml$',file):
            os.remove(file_fullpath)

    for file in os.listdir(decoders_dir):
        file_fullpath = os.path.join(decoders_dir, file)
        if os.path.isfile(file_fullpath) and re.match(r'^test_(.*?)_decoders.xml$',file):
            os.remove(file_fullpath)

def restart_analysisd():
    print("Restarting wazuh-manager...")
    ret = os.system('systemctl restart wazuh-manager')

def _get_win_base_rules():
    tree = ET.parse('/var/ossec/ruleset/rules/0575-win-base_rules.xml')
    return tree

def enable_win_eventlog_tests():
    tree = _get_win_base_rules()
    base_rule = tree.find('.//rule[@id="60000"]')
    base_rule.remove(base_rule.find(".//decoded_as"))
    base_rule.remove(base_rule.find(".//category"))
    decoded_as = ET.SubElement(base_rule,"decoded_as")
    decoded_as.text = "json"
    tree.write("/var/ossec/ruleset/rules/0575-win-base_rules.xml")
    restart_analysisd()

def disable_win_eventlog_tests():
    tree = _get_win_base_rules()
    base_rule = tree.find('.//rule[@id="60000"]')
    base_rule.remove(base_rule.find(".//decoded_as"))
    decoded_as = ET.SubElement(base_rule,"decoded_as")
    decoded_as.text = "windows_eventchannel"
    category = ET.SubElement(base_rule,"category")
    category.text = "ossec"
    tree.write("/var/ossec/ruleset/rules/0575-win-base_rules.xml")
    restart_analysisd()

class OssecTester(object):
    def __init__(self, bdir):
        self._error = False
        self._debug = False
        self._quiet = False
        self._ossec_path = bdir + "/bin/"
        self._test_path = "./tests"
        self._execution_data = {}
        self.tested_rules = set()
        self.tested_decoders = set()

    def buildCmd(self, rule, alert, decoder):
        cmd = ['%s/wazuh-logtest' % (self._ossec_path), ]
        cmd += ['-q']
        cmd += ['-U', "%s:%s:%s" % (rule, alert, decoder)]
        return cmd

    def runTest(self, log, rule, alert, decoder, section, name, negate=False):
        did_pass = "failed"
        self.tested_rules.add(rule)
        self.tested_decoders.add(decoder)
        p = subprocess.Popen(
            self.buildCmd(rule, alert, decoder),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            stdin=subprocess.PIPE,
            shell=False)
        std_out = p.communicate(log.encode())[0]
        if (p.returncode != 0 and not negate) or (p.returncode == 0 and negate):
            self._error = True
            print("")
            print("-" * 60)
            print("Failed: Exit code = %s" % (p.returncode))
            print("        Alert     = %s" % (alert))
            print("        Rule      = %s" % (rule))
            print("        Decoder   = %s" % (decoder))
            print("        Section   = %s" % (section))
            print("        line name = %s" % (name))
            print(" ")
            print(std_out)
        elif self._debug:
            print("Exit code= %s" % (p.returncode))
            print(std_out)
        else:
            sys.stdout.write(".")
            did_pass = "passed"
            sys.stdout.flush()
        return did_pass

    def run(self, selective_test=False, geoip=False, custom=False):
        for aFile in os.listdir(self._test_path):
            if re.match(r'^test_(.*?).ini$',aFile) and not custom:
                continue
            aFile = os.path.join(self._test_path, aFile)
            if aFile.endswith(".ini"):
                if selective_test and not aFile.endswith(selective_test):
                    continue
                if geoip is False and aFile.endswith("geoip.ini"):
                    continue
                self._execution_data[aFile] = {"passed":0, "failed":0}
                print("- [ File = %s ] ---------" % (aFile))
                tGroup = configparser.RawConfigParser(dict_type=MultiOrderedDict, strict=False)
                tGroup.read([aFile])
                tSections = tGroup.sections()
                for t in tSections:
                    rule = tGroup.get(t, "rule")
                    alert = tGroup.get(t, "alert")
                    decoder = tGroup.get(t, "decoder")
                    for (name, value) in tGroup.items(t):
                        if name.startswith("log "):
                            if self._debug:
                                print("-" * 60)
                            if name.endswith("pass"):
                                neg = False
                            elif name.endswith("fail"):
                                neg = True
                            else:
                                neg = False
                            self._execution_data[aFile][self.runTest(value, rule, alert, decoder,t, name, negate=neg)]+=1
                print("")
                print("")
        return self._error

    def print_results(self):
        template = "|{: ^25}|{: ^10}|{: ^10}|{: ^10}|"
        print(template.format("File", "Passed", "Failed", "Status"))
        print(template.format("--------", "--------", "--------", "--------"))
        template = "|{: <25}|{: ^10}|{: ^10}|{: ^10}|"
        for test_name in self._execution_data:
            passed_count = self._execution_data[test_name]["passed"]
            failed_count = self._execution_data[test_name]["failed"]
            status = u'\u274c' if (failed_count > 0) else u'\u2705'
            print(template.format(test_name, passed_count, failed_count, status))


def cleanup(*args):
    cleanDR()
    sys.exit(0)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='This script tests Wazuh rules.')
    parser.add_argument('--path', '-p', default='/var/ossec', dest='wazuh_home',
                        help='Use -p or --path to specify Wazuh installation path')
    parser.add_argument('--geoip', '-g', action='store_true', dest='geoip',
                        help='Use -g or --geoip to enable geoip tests (default: False)')
    parser.add_argument('--testfile', '-t', action='store', type=str, dest='testfile',
                        help='Use -t or --testfile to pass the ini file to test')
    parser.add_argument('--custom-ruleset', '-c', action='store_true', dest='custom',
                        help='Use -c or --custom-ruleset to test custom rules and decoders. WARNING: This will cause wazuh-manager restart')
    args = parser.parse_args()
    selective_test = False
    if args.testfile:
        selective_test = args.testfile
        if not selective_test.endswith('.ini'):
            selective_test += '.ini'

    wazuh_info = getWazuhInfo(args.wazuh_home)
    if wazuh_info is None:
        sys.exit(1)

    for sig in (signal.SIGABRT, signal.SIGINT, signal.SIGTERM):
        signal.signal(sig, cleanup)
    if args.custom:
        provisionDR()
        restart_analysisd()

    enable_win_eventlog_tests()

    OT = OssecTester(args.wazuh_home)
    error = OT.run(selective_test, args.geoip, args.custom)

    if args.custom:
        cleanDR()
        restart_analysisd()
    else:
        rules = getRuleIds("/var/ossec/ruleset/rules")
        decoders = getParentDecoderNames("/var/ossec/ruleset/decoders")

        template = "|{: ^10}|{: ^10}|{: ^10}|{: ^10}|"
        print(template.format("Component", "Tested", "Total", "Coverage"))
        print(template.format("--------", "--------", "--------", "--------"))
        template = "|{: ^10}|{: ^10}|{: ^10}|{: ^10.2%}|"
        print(template.format("Rules", len(OT.tested_rules), len(rules), len(OT.tested_rules)/len(rules)))
        print(template.format("Decoders", len(OT.tested_decoders), len(decoders), len(OT.tested_decoders)/len(decoders)))
        print("")

    OT.print_results()
    disable_win_eventlog_tests()
    if error:
        sys.exit(1)
