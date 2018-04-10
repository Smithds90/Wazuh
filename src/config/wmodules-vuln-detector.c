/*
 * Wazuh Module Configuration
 * Copyright (C) 2018 Wazuh Inc.
 * January, 2018.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef CLIENT
#ifndef WIN32
#include "wazuh_modules/wmodules.h"

static int get_interval(char *source, unsigned long *interval);
static int set_oval_version(char *feed, char *version, update_node **upd_list, update_node *upd);

//Options
static const char *XML_DISABLED = "disabled";
static const char *XML_INTERVAL = "interval";
static const char *XML_FEED = "feed";
static const char *XML_NAME = "name";
static const char *XML_UPDATE_INTERVAL = "update_interval";
static const char *XML_RUN_ON_START = "run_on_start";
static const char *XML_IGNORE_TIME = "ignore_time";
static const char *XML_URL = "url";
static const char *XML_PATH = "path";
static const char *XML_PORT = "port";
// Deprecated
static const char *XML_UPDATE_UBUNTU_OVAL = "update_ubuntu_oval";
static const char *XML_UPDATE_REDHAT_OVAL = "update_redhat_oval";
static const char *XML_VERSION = "version";

agent_software * skip_agent(agent_software *agents, agent_software **agents_list) {
    agent_software *next = NULL;
    if (agents->prev && agents->next) {
        next = agents->next;
        agents->prev->next = next;
        next->prev = agents->prev;
    } else if (agents->prev) {
        agents->prev->next = NULL;
    } else if (agents->next) {
        *agents_list = next = agents->next;
        next->prev = NULL;
    } else {
        *agents_list = NULL;
    }
    free(agents->agent_id);
    free(agents->agent_name);
    free(agents->agent_ip);
    free(agents);

    return next;
}

int set_oval_version(char *feed, char *version, update_node **upd_list, update_node *upd) {
    cve_db os_index;

    if (!strcmp(feed, vu_dist[DIS_UBUNTU])) {
        if (!strcmp(version, "12") || strcasestr(version, vu_dist[DIS_PRECISE])) {
            os_index = CVE_PRECISE;
            os_strdup(vu_dist[DIS_PRECISE], upd->version);
        } else if (!strcmp(version, "14") || strcasestr(version, vu_dist[DIS_TRUSTY])) {
            os_index = CVE_TRUSTY;
            os_strdup(vu_dist[DIS_TRUSTY], upd->version);
        } else if (!strcmp(version, "16") || strcasestr(version, vu_dist[DIS_XENIAL])) {
            os_index = CVE_XENIAL;
            os_strdup(vu_dist[DIS_XENIAL], upd->version);
        } else {
            merror("Invalid Ubuntu version '%s'.", version);
            return OS_INVALID;
        }
    } else if (!strcmp(feed, vu_dist[DIS_REDHAT])) {
        if (!strcmp(version, "5")) {
            os_index = CVE_RHEL5;
        } else if (!strcmp(version, "6")) {
            os_index = CVE_RHEL6;
        } else if (!strcmp(version, "7")) {
            os_index = CVE_RHEL7;
        } else {
            merror("Invalid Redhat version '%s'.", version);
            return OS_INVALID;
        }
    } else {
        merror("Invalid OS for tag '%s' at module '%s'.", XML_FEED, WM_VULNDETECTOR_CONTEXT.name);
        return OS_INVALID;
    }

    os_strdup(feed, upd->dist);
    if (!upd->version) {
        os_strdup(version, upd->version);
    }

    if (upd_list[os_index]) {
        mwarn("Duplicate OVAL configuration for '%s %s'.", upd->dist, upd->version);
        free(upd->dist);
        free(upd->version);
        free(upd);
        return OS_SUPP_SIZE;
    }

    upd_list[os_index] = upd;

    upd->url = NULL;
    upd->path = NULL;
    upd->port = 0;

    return os_index;
}

int get_interval(char *source, unsigned long *interval) {
    char *endptr;
    *interval = strtoul(source, &endptr, 0);

    if ((!*interval && endptr == source) || *interval == ULONG_MAX) {
        return OS_INVALID;
    }

    switch (*endptr) {
    case 'd':
        *interval *= 86400;
        break;
    case 'h':
        *interval *= 3600;
        break;
    case 'm':
        *interval *= 60;
        break;
    case 's':
    case '\0':
        break;
    default:
        return OS_INVALID;
    }

    return 0;
}

int wm_vulnerability_detector_read(const OS_XML *xml, xml_node **nodes, wmodule *module) {
    unsigned int i, j;
    wm_vulnerability_detector_t * vulnerability_detector;
    XML_NODE chld_node = NULL;

    os_calloc(1, sizeof(wm_vulnerability_detector_t), vulnerability_detector);
    vulnerability_detector->flags.run_on_start = 1;
    vulnerability_detector->flags.enabled = 1;
    vulnerability_detector->flags.u_flags.update = 0;
    vulnerability_detector->flags.u_flags.update_ubuntu = 0;
    vulnerability_detector->flags.u_flags.update_redhat = 0;
    vulnerability_detector->ignore_time = VU_DEF_IGNORE_TIME;
    vulnerability_detector->detection_interval = WM_VULNDETECTOR_DEFAULT_INTERVAL;
    vulnerability_detector->agents_software = NULL;
    module->context = &WM_VULNDETECTOR_CONTEXT;
    module->data = vulnerability_detector;

    for (i = 0; i < OS_SUPP_SIZE; i++) {
        vulnerability_detector->updates[i] = NULL;
    }

    for (i = 0; nodes[i]; i++) {
        if (!nodes[i]->element) {
            merror(XML_ELEMNULL);
            return OS_INVALID;
        } else if (!strcmp(nodes[i]->element, XML_DISABLED)) {
            if (!strcmp(nodes[i]->content, "yes"))
                vulnerability_detector->flags.enabled = 0;
            else if (!strcmp(nodes[i]->content, "no")) {
                vulnerability_detector->flags.enabled = 1;
            } else {
                merror("Invalid content for tag '%s' at module '%s'.", XML_DISABLED, WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }
        } else if (!strcmp(nodes[i]->element, XML_INTERVAL)) {
            if (get_interval(nodes[i]->content, &vulnerability_detector->detection_interval)) {
                merror("Invalid interval at module '%s'", WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }
        } else if (!strcmp(nodes[i]->element, XML_FEED)) {
            char *feed;
            char *version;
            int os_index;
            update_node *upd;

            if (!nodes[i]->attributes || strcmp(*nodes[i]->attributes, XML_NAME)) {
                merror("Invalid content for tag '%s' at module '%s'.", XML_FEED, WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }
            for (j = 0; *(feed = &nodes[i]->values[0][j]) != '\0'; j++) {
                if (isalpha(*feed)) {
                    *feed = toupper(*feed);
                }
            }
            feed = nodes[i]->values[0];
            if (version = strchr(feed, '-'), version) {
                *version = '\0';
                version++;
            } else {
                merror("Invalid OS for tag '%s' at module '%s'.", XML_FEED, WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }

            os_calloc(1, sizeof(update_node), upd);

            if (os_index = set_oval_version(feed, version, vulnerability_detector->updates, upd), os_index == OS_INVALID) {
                return OS_INVALID;
            } else if (os_index == OS_SUPP_SIZE) {
                continue;
            }

            if (chld_node = OS_GetElementsbyNode(xml, nodes[i]), !chld_node) {
                merror(XML_INVELEM, nodes[i]->element);
                return OS_INVALID;
            }

            for (j = 0; chld_node[j]; j++) {
                if (!strcmp(chld_node[j]->element, XML_DISABLED)) {
                    if (!strcmp(chld_node[j]->content, "yes")) {
                        free(upd->dist);
                        free(upd->version);
                        if (upd->url) {
                            free(upd->url);
                        }
                        if (upd->path) {
                            free(upd->path);
                        }
                        free(upd);
                        vulnerability_detector->updates[os_index] = NULL;
                        break;
                    } else if (!strcmp(chld_node[j]->content, "no")) {
                        if (!strcmp(upd->dist, vu_dist[DIS_REDHAT])) {
                            vulnerability_detector->flags.u_flags.update_redhat = 1;
                        } else if (!strcmp(upd->dist, vu_dist[DIS_UBUNTU])) {
                            vulnerability_detector->flags.u_flags.update_ubuntu = 1;
                        }
                    } else {
                        merror("Invalid content for '%s' option at module '%s'", XML_DISABLED, WM_VULNDETECTOR_CONTEXT.name);
                        OS_ClearNode(chld_node);
                        return OS_INVALID;
                    }
                } else if (!strcmp(chld_node[j]->element, XML_UPDATE_INTERVAL)) {
                    if (get_interval(chld_node[j]->content, &upd->interval)) {
                        merror("Invalid content for '%s' option at module '%s'", XML_UPDATE_INTERVAL, WM_VULNDETECTOR_CONTEXT.name);
                        OS_ClearNode(chld_node);
                        return OS_INVALID;
                    }
                } else if (!strcmp(chld_node[j]->element, XML_URL)) {
                    os_strdup(chld_node[j]->content, upd->url);
                    if (chld_node[j]->attributes && !strcmp(*chld_node[j]->attributes, XML_PORT)) {
                        upd->port = strtol(*chld_node[j]->values, NULL, 10);
                    }
                } else if (!strcmp(chld_node[j]->element, XML_PATH)) {
                    os_strdup(chld_node[j]->content, upd->path);
                } else {
                    merror("Invalid option '%s' for tag '%s' at module '%s'.", chld_node[j]->element, XML_FEED , WM_VULNDETECTOR_CONTEXT.name);
                    OS_ClearNode(chld_node);
                    return OS_INVALID;
                }
            }

            OS_ClearNode(chld_node);
        } else if (!strcmp(nodes[i]->element, XML_RUN_ON_START)) {
            if (!strcmp(nodes[i]->content, "yes")) {
                vulnerability_detector->flags.run_on_start = 1;
            } else if (!strcmp(nodes[i]->content, "no")) {
                vulnerability_detector->flags.run_on_start = 0;
            } else {
                merror("Invalid content for tag '%s' at module '%s'.", XML_RUN_ON_START, WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }
        } else if (!strcmp(nodes[i]->element, XML_IGNORE_TIME)) {
            if (get_interval(nodes[i]->content, &vulnerability_detector->ignore_time)) {
                merror("Invalid ignore_time at module '%s'", WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }
        } else if (!strcmp(nodes[i]->element, XML_UPDATE_UBUNTU_OVAL)) {
            int enabled = 0;
            long unsigned int interval = 0;
            int precise = 0, trusty = 0, xenial = 0;
            mwarn("'%s' option at module '%s' is deprecated. Use '%s' instead.", nodes[i]->element, WM_VULNDETECTOR_CONTEXT.name, XML_FEED);

            if (!strcmp(nodes[i]->content, "yes")) {
                enabled = 1;
                if (nodes[i]->attributes) {
                    for (j = 0; nodes[i]->attributes[j]; j++) {
                        if (!strcmp(nodes[i]->attributes[j], XML_VERSION)) {
                            int k;
                            char * version = nodes[i]->values[j];
                            for (k = 0;; k++) {
                                int out = (version[k] == '\0');
                                if (version[k] == ',' || out) {
                                    version[k] = '\0';
                                    if (!strcmp(version, "12")) {
                                        precise = 1;
                                    } else if (!strcmp(version, "14")) {
                                        trusty = 1;
                                    } else if (!strcmp(version, "16")) {
                                        xenial = 1;
                                    } else {
                                        merror("Invalid Ubuntu version '%s'.", version);
                                    }
                                    version = &version[k] + 1;
                                    k = 0;
                                }
                                if (out)
                                    break;
                            }
                        } else if (!strcmp(nodes[i]->attributes[j], XML_INTERVAL)) {
                            if (get_interval(nodes[i]->values[j], &interval)) {
                                merror("Invalid interval at module '%s'", WM_VULNDETECTOR_CONTEXT.name);
                                return OS_INVALID;
                            }
                        } else {
                            merror("Invalid attribute '%s' for '%s'", nodes[i]->attributes[j], XML_UPDATE_UBUNTU_OVAL);
                        }
                    }
                }
            } else if (!strcmp(nodes[i]->content, "no")) {
                enabled = 0;
            } else {
                merror("Invalid content for tag '%s' at module '%s'.", XML_RUN_ON_START, WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }

            if (enabled) {
                int os_index;
                update_node *upd;

                if (precise) {
                    os_calloc(1, sizeof(update_node), upd);
                    if (os_index = set_oval_version("UBUNTU", "12", vulnerability_detector->updates, upd), os_index == OS_INVALID) {
                        return OS_INVALID;
                    } else if (os_index == OS_SUPP_SIZE) {
                        continue;
                    }
                    upd->interval = interval;
                }
                if (trusty) {
                    os_calloc(1, sizeof(update_node), upd);
                    if (os_index = set_oval_version("UBUNTU", "14", vulnerability_detector->updates, upd), os_index == OS_INVALID) {
                        return OS_INVALID;
                    } else if (os_index == OS_SUPP_SIZE) {
                        continue;
                    }
                    upd->interval = interval;
                }
                if (xenial) {
                    os_calloc(1, sizeof(update_node), upd);
                    if (os_index = set_oval_version("UBUNTU", "16", vulnerability_detector->updates, upd), os_index == OS_INVALID) {
                        return OS_INVALID;
                    } else if (os_index == OS_SUPP_SIZE) {
                        continue;
                    }
                    upd->interval = interval;
                }
            }
        } else if (!strcmp(nodes[i]->element, XML_UPDATE_REDHAT_OVAL)) {
            int enabled = 0;
            long unsigned int interval = 0;
            int rhel5 = 0, rhel6 = 0, rhel7 = 0;
            mwarn("'%s' option at module '%s' is deprecated. Use '%s' instead.", nodes[i]->element, WM_VULNDETECTOR_CONTEXT.name, XML_FEED);

            if (!strcmp(nodes[i]->content, "yes")) {
                enabled = 1;
                if (nodes[i]->attributes) {
                    for (j = 0; nodes[i]->attributes[j]; j++) {
                        if (!strcmp(nodes[i]->attributes[j], XML_VERSION)) {
                            int k;
                            char * version = nodes[i]->values[j];
                            for (k = 0;; k++) {
                                int out = (version[k] == '\0');
                                if (version[k] == ',' || out) {
                                    version[k] = '\0';
                                    if (!strcmp(version, "5")) {
                                        rhel5 = 1;
                                    } else if (!strcmp(version, "6")) {
                                        rhel6 = 1;
                                    } else if (!strcmp(version, "7")) {
                                        rhel7 = 1;
                                    } else {
                                        merror("Invalid RedHat version '%s'.", version);
                                    }
                                    version = &version[k] + 1;
                                    k = 0;
                                }
                                if (out)
                                    break;
                            }
                        } else if (!strcmp(nodes[i]->attributes[j], XML_INTERVAL)) {
                            if (get_interval(nodes[i]->values[j], &interval)) {
                                merror("Invalid interval at module '%s'", WM_VULNDETECTOR_CONTEXT.name);
                                return OS_INVALID;
                            }
                        } else {
                            merror("Invalid attribute '%s' for '%s'", nodes[i]->attributes[j], XML_UPDATE_REDHAT_OVAL);
                        }
                    }
                }
            } else if (!strcmp(nodes[i]->content, "no")) {
                enabled = 0;
            } else {
                merror("Invalid content for tag '%s' at module '%s'.", XML_RUN_ON_START, WM_VULNDETECTOR_CONTEXT.name);
                return OS_INVALID;
            }

            if (enabled) {
                int os_index;
                update_node *upd;

                if (rhel5) {
                    os_calloc(1, sizeof(update_node), upd);
                    if (os_index = set_oval_version("REDHAT", "5", vulnerability_detector->updates, upd), os_index == OS_INVALID) {
                        return OS_INVALID;
                    } else if (os_index == OS_SUPP_SIZE) {
                        continue;
                    }
                    upd->interval = interval;
                }
                if (rhel6) {
                    os_calloc(1, sizeof(update_node), upd);
                    if (os_index = set_oval_version("REDHAT", "6", vulnerability_detector->updates, upd), os_index == OS_INVALID) {
                        return OS_INVALID;
                    } else if (os_index == OS_SUPP_SIZE) {
                        continue;
                    }
                    upd->interval = interval;
                }
                if (rhel7) {
                    os_calloc(1, sizeof(update_node), upd);
                    if (os_index = set_oval_version("REDHAT", "7", vulnerability_detector->updates, upd), os_index == OS_INVALID) {
                        return OS_INVALID;
                    } else if (os_index == OS_SUPP_SIZE) {
                        continue;
                    }
                    upd->interval = interval;
                }
            }
        } else {
            merror("No such tag '%s' at module '%s'.", nodes[i]->element, WM_VULNDETECTOR_CONTEXT.name);
            return OS_INVALID;
        }
    }

    if (vulnerability_detector->flags.u_flags.update_ubuntu || vulnerability_detector->flags.u_flags.update_redhat) {
        vulnerability_detector->flags.u_flags.update = 1;
    }

    return 0;
}

#endif
#endif
