/* Copyright (C) 2015-2021, Wazuh Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "manage_agents_wrappers.h"
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>

double __wrap_get_time_since_agent_disconnection(__attribute__((unused)) const char *id) {
    return mock();
}

void __wrap_OS_RemoveAgentGroup(__attribute__((unused)) const char *id) {
    // Empty wrapper
}

time_t __wrap_get_time_since_agent_registration(__attribute__((unused)) int id) {
    return mock();
}
