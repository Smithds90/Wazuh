/*
 * Wazuh Syscheck Module Configuration
 * Copyright (C) 2015-2021, Wazuh Inc.
 * March 31, 2021.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "wazuh_modules/wmodules.h"
#include "syscheck-config.h"
#include "config.h"

#ifdef WIN32
static char *SYSCHECK_EMPTY[] = { NULL };
static registry REGISTRY_EMPTY[] = { { NULL, 0, 0, 512, 0, NULL, NULL, NULL} };
#endif

// Parse XML configuration
int Read_Syscheck(const OS_XML *xml, XML_NODE node, void* d1, int modules, char alloc) {
    wmodule** wmodules = (wmodule**)d1;
    wmodule* cur_wmodule = NULL;
    syscheck_config* config = NULL;
    // Allocate memory
    if(alloc) {
        if ((cur_wmodule = *wmodules)) {
            while (cur_wmodule->next)
                cur_wmodule = cur_wmodule->next;

            os_calloc(1, sizeof(wmodule), cur_wmodule->next);
            cur_wmodule = cur_wmodule->next;
        } else
            *wmodules = cur_wmodule = calloc(1, sizeof(wmodule));

        if (!cur_wmodule) {
            merror(MEM_ERROR, errno, strerror(errno));
            return (OS_INVALID);
        }
        if(!cur_wmodule->data) {
            os_calloc(1, sizeof(syscheck_config), config);
            memset(config, 0, sizeof(syscheck_config));
            cur_wmodule->context = &WM_SYSCHECK_CONTEXT;
            cur_wmodule->tag = strdup(cur_wmodule->context->name);
            cur_wmodule->data = config;
        }
        config = cur_wmodule->data;
    }
    else {
        config = d1;
    }
    int it = 0;

    config->rootcheck      = 0;
    config->disabled       = SK_CONF_UNPARSED;
    config->database_store = FIM_DB_DISK;
    config->skip_fs.nfs    = 1;
    config->skip_fs.dev    = 1;
    config->skip_fs.sys    = 1;
    config->skip_fs.proc   = 1;
    config->scan_on_start  = 1;
    config->time           = 43200;
    config->ignore         = NULL;
    config->ignore_regex   = NULL;
    config->nodiff         = NULL;
    config->nodiff_regex   = NULL;
    config->scan_day       = NULL;
    config->scan_time      = NULL;
    config->file_limit_enabled = true;
    config->file_limit     = 100000;
    config->dir            = NULL;
    config->opts           = NULL;
    config->enable_synchronization = 1;
    config->restart_audit  = 1;
    config->enable_whodata = 0;
    config->realtime       = NULL;
    config->audit_healthcheck = 1;
    config->process_priority = 10;
#ifdef WIN_WHODATA
    config->wdata.interval_scan = 0;
    config->wdata.fd      = NULL;
#endif
#ifdef WIN32
    config->realtime_change = 0;
    config->registry       = NULL;
    config->key_ignore = NULL;
    config->key_ignore_regex = NULL;
    config->value_ignore = NULL;
    config->value_ignore_regex = NULL;
    config->max_fd_win_rt  = 0;
    config->registry_nodiff = NULL;
    config->registry_nodiff_regex = NULL;
    config->enable_registry_synchronization = 1;
#endif
    config->prefilter_cmd  = NULL;
    config->sync_interval  = 300;
    config->max_sync_interval = 3600;
    config->sync_response_timeout = 30;
    config->sync_queue_size = 16384;
    config->sync_max_eps = 10;
    config->max_eps        = 100;
    config->max_files_per_second = 0;
    config->allow_remote_prefilter_cmd  = false;
    config->disk_quota_enabled = true;
    config->disk_quota_limit = 1024 * 1024; // 1 GB
    config->file_size_enabled = true;
    config->file_size_limit = 50 * 1024; // 50 MB
    config->diff_folder_size = 0;
    config->comp_estimation_perc = 0.9;    // 90%
    config->disk_quota_full_msg = true;
    config->audit_key = NULL;

    /* Read config */
    if (node && read_syscheck_config_xml(xml, node, config, modules) < 0) {
        return (OS_INVALID);
    }

#ifdef CLIENT

    /* Read shared config */
    modules |= CAGENT_CONFIG;
    ReadConfig(modules, AGENTCONFIG, config, NULL);
#endif

    // Check directories options to determine whether to start the whodata thread or not
    if (config->dir) {
        for (it = 0; config->dir[it]; it++) {
            if (config->opts[it] & WHODATA_ACTIVE) {
                config->enable_whodata = 1;

                break;  // Exit loop with the first whodata directory
            }
        }
    }

    if (config->diff_size_limit) {
        for (it = 0; config->diff_size_limit[it]; it++) {
            if (config->diff_size_limit[it] == -1) {
                config->diff_size_limit[it] = config->file_size_limit;
            }
        }
    }

    switch (config->disabled) {
    case SK_CONF_UNPARSED:
        config->disabled = 1;
        break;
    case SK_CONF_UNDEFINED:
        config->disabled = 0;
    }

#ifndef WIN32
    /* We must have at least one directory to check */
    if (!config->dir || config->dir[0] == NULL) {
        return (1);
    }
#else
    /* We must have at least one directory or registry key to check. Since
       it's possible on Windows to have syscheck enabled but only monitoring
       either the filesystem or the registry, both lists must be valid,
       even if empty.
     */
    if (!config->dir) {
        config->dir = SYSCHECK_EMPTY;
    }
    if (!config->registry) {
            config->registry = REGISTRY_EMPTY;
    } else {
        it = 0;
        while (config->registry[it].entry) {
            if (config->registry[it].diff_size_limit == -1) {
                config->registry[it].diff_size_limit = config->file_size_limit;
            }
            it++;
        }
    }
    if ((config->dir[0] == NULL) && (config->registry[0].entry == NULL)) {
        return (1);
    }
    config->max_fd_win_rt = getDefine_Int("syscheck", "max_fd_win_rt", 1, 1024);
#endif

    return (0);
}
