ARG ENVIRONMENT

FROM ubuntu:18.04 AS base

ARG wazuhbranch

RUN apt-get update && apt-get install -y supervisor
ADD base/wazuh-manager/supervisord.conf /etc/supervisor/conf.d/

RUN apt-get update && apt-get install python git gnupg2 gcc make vim libc6-dev curl policycoreutils automake autoconf libtool apt-transport-https lsb-release python-cryptography -y && curl -s https://packages.wazuh.com/key/GPG-KEY-WAZUH | apt-key add - && echo "deb https://s3-us-west-1.amazonaws.com/packages-dev.wazuh.com/staging/apt/ unstable main" | tee -a /etc/apt/sources.list.d/wazuh.list

RUN git clone https://github.com/wazuh/wazuh && cd /wazuh && git checkout $wazuhbranch
COPY base/wazuh-manager/preloaded-vars.conf /wazuh/etc/preloaded-vars.conf
RUN /wazuh/install.sh

#####

ONBUILD COPY configurations/base/wazuh-master/config/ossec.conf /var/ossec/etc/ossec.conf
ONBUILD COPY configurations/base/wazuh-master/config/test.keys /var/ossec/etc/client.keys
ONBUILD COPY configurations/base/wazuh-master/config/agent-groups /var/ossec/queue/agent-groups
ONBUILD COPY configurations/base/wazuh-master/config/shared /var/ossec/etc/shared
ONBUILD COPY configurations/base/wazuh-master/config/agent-info /var/ossec/queue/agent-info
ONBUILD COPY configurations/base/wazuh-master/healthcheck/healthcheck.py /tmp/healthcheck.py
ONBUILD COPY configurations/base/wazuh-master/healthcheck/agent_control_check.txt /tmp/agent_control_check.txt
ADD base/wazuh-manager/entrypoint.sh /scripts/entrypoint.sh

FROM base AS wazuh-env-base
FROM base AS wazuh-env-ciscat
FROM base AS wazuh-env-syscollector

FROM base AS wazuh-env-security
ONBUILD COPY configurations/security/wazuh-master/rbac.db /var/ossec/api/configuration/security/rbac.db

FROM base AS wazuh-env-manager
ONBUILD COPY configurations/manager/wazuh-master/ossec-totals-27.log /var/ossec/stats/totals/2019/Aug/ossec-totals-27.log

FROM base AS wazuh-env-cluster
ONBUILD COPY configurations/cluster/wazuh-master/ossec-totals-27.log /var/ossec/stats/totals/2019/Aug/ossec-totals-27.log

FROM wazuh-env-${ENVIRONMENT}

HEALTHCHECK --retries=30 --interval=10s --timeout=30s --start-period=30s CMD /usr/bin/python3 /tmp/healthcheck.py || exit 1

