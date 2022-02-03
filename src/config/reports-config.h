/* Copyright (C) 2015, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#ifndef REPORTSCONFIG_H
#define REPORTSCONFIG_H

#include "report_op.h"
#include "global-config.h"

/* Structure for the report */
typedef struct _report_config {
    char *title;
    char *args;
    char *relations;
    char *type;
    char **emailto;
    report_filter r_filter;
} report_config;

typedef struct _delete_agents{
    time_t older_than; /*in seconds*/
    time_t interval; /*in seconds*/
    int enabled;
    char** status_list;
    char** os_name_list;
}delete_agents_t;

typedef struct _monitor_config {
    short int day_wait;
    unsigned int compress:1;
    unsigned int sign:1;
    unsigned int monitor_agents:1;
    unsigned int rotate_log:1;
    int a_queue;
    int keep_log_days;
    unsigned long size_rotate;
    int daily_rotations;

    char *smtpserver;
    char *emailfrom;
    char *emailidsname;

    char **agents;
    report_config **reports;
    delete_agents_t delete_agents;

    _Config global;
} monitor_config;

#endif /* REPORTSCONFIG_H */
