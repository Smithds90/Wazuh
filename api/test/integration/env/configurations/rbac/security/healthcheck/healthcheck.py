import os

os.system("/var/ossec/bin/ossec-control status > /tmp/output.txt")
check = os.popen("diff -q /tmp/output.txt /tmp/security_rbac.txt").read()

if "differ" in check:
    exit(0)
else:
    exit(1)
