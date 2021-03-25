/*
 * Wazuh DB helper module for agents database
 * Copyright (C) 2015-2021, Wazuh Inc.
 * February 10, 2021.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef WDB_AGENTS_H
#define WDB_AGENTS_H

#include "wdb.h"

/**
 * @brief Function to insert a new entry into the agent vuln_cves table.
 *
 * @param [in] wdb The 'agents' struct database.
 * @param [in] name The vulnerable package name.
 * @param [in] version The vulnerable package version.
 * @param [in] architecture The vulnerable package architecture.
 * @param [in] cve The CVE id of the vulnerability.
 * @return Returns 0 on success or -1 on error.
 */
int wdb_agents_insert_vuln_cves(wdb_t *wdb, const char* name, const char* version, const char* architecture, const char* cve);

/**
 * @brief Function to update the status field in agent database vuln_cves table.
 *
 * @param [in] wdb The 'agents' struct database.
 * @param [in] old_status The status that is going to be updated. The '*' option changes all statuses.
 * @param [in] new_status The new status.
 * @return Returns 0 on success or -1 on error.
 */
int wdb_agents_update_status_vuln_cves(wdb_t *wdb, const char* old_status, const char* new_status);

/**
 * @brief Function to remove vulnerabilities from the vuln_cves table by specifying the PK of the entry.
 *
 * @param [in] wdb The 'agents' struct database.
 * @param [in] cve The cve of the entry that should be removed.
 * @param [in] reference The reference of the entry that should be removed.
 * @return Returns 0 on success or -1 on error.
 */
int wdb_agents_remove_vuln_cves(wdb_t *wdb, const char* cve, const char* reference);

/**
 * @brief Function to remove vulnerabilities from the vuln_cves table filtering by the status.
 *
 * @param [in] wdb The 'agents' struct database.
 * @param [in] status The status that is going to be updated. The '*' option changes all statuses.
 * @param [out] output A buffer where the response is written. Must be de-allocated by the caller.
 * @return wdbc_result to represent if all the vulnerabilities have been removed.
 */
wdbc_result wdb_agents_remove_by_status_vuln_cves(wdb_t *wdb, const char* status, char **output);

/**
 * @brief Function to clear whole data from agent vuln_cves table.
 *
 * @param [in] wdb The 'agents' struct database.
 * @return Returns 0 on success or -1 on error.
 */
int wdb_agents_clear_vuln_cves(wdb_t *wdb);

#endif
