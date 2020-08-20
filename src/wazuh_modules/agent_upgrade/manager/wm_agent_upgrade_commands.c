/*
 * Wazuh Module for Agent Upgrading
 * Copyright (C) 2015-2020, Wazuh Inc.
 * July 3, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */
#include "wazuh_modules/wmodules.h"
#include "wm_agent_upgrade_manager.h"
#include "wm_agent_upgrade_parsing.h"
#include "wm_agent_upgrade_tasks.h"
#include "wm_agent_upgrade_validate.h"
#include "wazuh_db/wdb.h"
#include "os_net/os_net.h"

/**
 * Analyze agent information and returns a JSON to be sent to the task manager
 * @param agent_id id of the agent
 * @param agent_task structure where the information of the agent will be stored
 * @param error_code variable to modify in case of failure
 * @param manager_configs manager configuration parameters
 * @return JSON task if success, NULL otherwise
 * */
static cJSON* wm_agent_upgrade_analyze_agent(int agent_id, wm_agent_task *agent_task, wm_upgrade_error_code *error_code, const wm_manager_configs* manager_configs);

/**
 * Validate the information of the agent and the task
 * @param agent_task structure with the information to be validated
 * @param manager_configs manager configuration parameters
 * @return return_code
 * @retval WM_UPGRADE_SUCCESS
 * @retval WM_UPGRADE_GLOBAL_DB_FAILURE
 * @retval WM_UPGRADE_INVALID_ACTION_FOR_MANAGER
 * @retval WM_UPGRADE_AGENT_IS_NOT_ACTIVE
 * @retval WM_UPGRADE_UPGRADE_ALREADY_IN_PROGRESS
 * @retval WM_UPGRADE_NOT_MINIMAL_VERSION_SUPPORTED
 * @retval WM_UPGRADE_SYSTEM_NOT_SUPPORTED
 * @retval WM_UPGRADE_URL_NOT_FOUND
 * @retval WM_UPGRADE_WPK_VERSION_DOES_NOT_EXIST
 * @retval WM_UPGRADE_NEW_VERSION_LEES_OR_EQUAL_THAT_CURRENT
 * @retval WM_UPGRADE_NEW_VERSION_GREATER_MASTER
 * @retval WM_UPGRADE_VERSION_SAME_MANAGER
 * @retval WM_UPGRADE_WPK_FILE_DOES_NOT_EXIST
 * @retval WM_UPGRADE_WPK_SHA1_DOES_NOT_MATCH
 * @retval WM_UPGRADE_UNKNOWN_ERROR
 * */
static int wm_agent_upgrade_validate_agent_task(const wm_agent_task *agent_task, const wm_manager_configs* manager_configs);

/**
 * Start the upgrade procedure for the agents
 * @param json_response cJSON array where the responses for each agent will be stored
 * @param task_module_request cJSON array with the agents to be upgraded
 * @param manager_configs manager configuration parameters
 * */
static void wm_agent_upgrade_start_upgrades(cJSON *json_response, const cJSON* task_module_request, const wm_manager_configs* manager_configs);

/**
 * Send WPK file to agent and verify SHA1
 * @param agent_task structure with the information of the agent and the WPK
 * @param manager_configs manager configuration parameters
 * @return error code
 * @retval OS_SUCCESS on success
 * @retval OS_INVALID on errors
 * */
static int wm_agent_upgrade_send_wpk_to_agent(const wm_agent_task *agent_task, const wm_manager_configs* manager_configs);

/**
 * Send a lock_restart command to an agent
 * @param agent_id id of the agent
 * @return error code
 * @retval OS_SUCCESS on success
 * @retval OS_INVALID on errors
 * */
static int wm_agent_upgrade_send_lock_restart(int agent_id);

/**
 * Send an open file command to an agent
 * @param agent_id id of the agent
 * @param wpk_file name of the file to open in the agent
 * @return error code
 * @retval OS_SUCCESS on success
 * @retval OS_INVALID on errors
 * */
static int wm_agent_upgrade_send_open(int agent_id, const char *wpk_file);

/**
 * Send a write file command to an agent
 * @param agent_id id of the agent
 * @param wpk_file name of the file to write in the agent
 * @param file_path name of the file to read in the manager
 * @param chunk_size size of block to send WPK file
 * @return error code
 * @retval OS_SUCCESS on success
 * @retval OS_INVALID on errors
 * */
static int wm_agent_upgrade_send_write(int agent_id, const char *wpk_file, const char *file_path, int chunk_size);

/**
 * Send a close file command to an agent
 * @param agent_id id of the agent
 * @param wpk_file name of the file to close in the agent
 * @return error code
 * @retval OS_SUCCESS on success
 * @retval OS_INVALID on errors
 * */
static int wm_agent_upgrade_send_close(int agent_id, const char *wpk_file);

/**
 * Send a sha1 command to an agent
 * @param agent_id id of the agent
 * @param wpk_file name of the file to calculate sha1 in the agent
 * @param file_sha1 sha1 of the file in the manager to compare
 * @return error code
 * @retval OS_SUCCESS on success
 * @retval OS_INVALID on errors
 * */
static int wm_agent_upgrade_send_sha1(int agent_id, const char *wpk_file, const char *file_sha1);

/**
 * Send an upgrade command to an agent
 * @param agent_id id of the agent
 * @param wpk_file name of the file with the installation files in the agent
 * @param installer name of the installer to run in the agent
 * @return error code
 * @retval OS_SUCCESS on success
 * @retval OS_INVALID on errors
 * */
static int wm_agent_upgrade_send_upgrade(int agent_id, const char *wpk_file, const char *installer);

/**
 * Sends a single message to the task module and returns a response
 * @param command wm_upgrade_command that will be used to generate the message
 * @param agent_id id of the agent
 * @param status in case the command is an status update
 * @return cJSON with the response from the task manager
 * */
static cJSON* wm_agent_upgrade_send_single_task(wm_upgrade_command command, int agent_id, const char* status_task);

char* wm_agent_upgrade_process_upgrade_command(const int* agent_ids, wm_upgrade_task* task, const wm_manager_configs* manager_configs) {
    char* response = NULL;
    int agent = 0;
    int agent_id = 0;
    cJSON* json_response = cJSON_CreateArray();
    cJSON *json_task_module_request = cJSON_CreateArray();

    while (agent_id = agent_ids[agent++], agent_id != OS_INVALID) {
        wm_upgrade_error_code error_code = WM_UPGRADE_SUCCESS;
        cJSON *task_request = NULL;
        wm_agent_task *agent_task = NULL;
        wm_upgrade_task *upgrade_task = NULL;

        agent_task = wm_agent_upgrade_init_agent_task();

        // Task information
        upgrade_task = wm_agent_upgrade_init_upgrade_task();
        w_strdup(task->wpk_repository, upgrade_task->wpk_repository);
        w_strdup(task->custom_version, upgrade_task->custom_version);
        upgrade_task->use_http = task->use_http;
        upgrade_task->force_upgrade = task->force_upgrade;

        agent_task->task_info = wm_agent_upgrade_init_task_info();
        agent_task->task_info->command = WM_UPGRADE_UPGRADE;
        agent_task->task_info->task = upgrade_task;

        if (task_request = wm_agent_upgrade_analyze_agent(agent_id, agent_task, &error_code, manager_configs), task_request) {
            cJSON_AddItemToArray(json_task_module_request, task_request);
        } else {
            cJSON *error_message = wm_agent_upgrade_parse_response_message(error_code, upgrade_error_codes[error_code], &agent_id, NULL, NULL);
            cJSON_AddItemToArray(json_response, error_message);
            wm_agent_upgrade_free_agent_task(agent_task);
        }
    }

    wm_agent_upgrade_start_upgrades(json_response, json_task_module_request, manager_configs);

    response = cJSON_PrintUnformatted(json_response);

    cJSON_Delete(json_task_module_request);
    cJSON_Delete(json_response);

    return response;
}

char* wm_agent_upgrade_process_upgrade_custom_command(const int* agent_ids, wm_upgrade_custom_task* task, const wm_manager_configs* manager_configs) {
    char* response = NULL;
    int agent = 0;
    int agent_id = 0;
    cJSON* json_response = cJSON_CreateArray();
    cJSON *json_task_module_request = cJSON_CreateArray();

    while (agent_id = agent_ids[agent++], agent_id != OS_INVALID) {
        wm_upgrade_error_code error_code = WM_UPGRADE_SUCCESS;
        cJSON *task_request = NULL;
        wm_agent_task *agent_task = NULL;
        wm_upgrade_custom_task *upgrade_custom_task = NULL;

        agent_task = wm_agent_upgrade_init_agent_task();

        // Task information
        upgrade_custom_task = wm_agent_upgrade_init_upgrade_custom_task();
        w_strdup(task->custom_file_path, upgrade_custom_task->custom_file_path);
        w_strdup(task->custom_installer, upgrade_custom_task->custom_installer);

        agent_task->task_info = wm_agent_upgrade_init_task_info();
        agent_task->task_info->command = WM_UPGRADE_UPGRADE_CUSTOM;
        agent_task->task_info->task = upgrade_custom_task;

        if (task_request = wm_agent_upgrade_analyze_agent(agent_id, agent_task, &error_code, manager_configs), task_request) {
            cJSON_AddItemToArray(json_task_module_request, task_request);
        } else {
            cJSON *error_message = wm_agent_upgrade_parse_response_message(error_code, upgrade_error_codes[error_code], &agent_id, NULL, NULL);
            cJSON_AddItemToArray(json_response, error_message);
            wm_agent_upgrade_free_agent_task(agent_task);
        }
    }

    wm_agent_upgrade_start_upgrades(json_response, json_task_module_request, manager_configs);

    response = cJSON_PrintUnformatted(json_response);

    cJSON_Delete(json_task_module_request);
    cJSON_Delete(json_response);

    return response;
}

char* wm_agent_upgrade_process_agent_result_command(const int* agent_ids, wm_upgrade_agent_status_task* task) {
    // Only one id of agent will reach at a time
    int agent_id = 0;
    int agent = 0;
    cJSON* json_response = cJSON_CreateArray();
    cJSON *json_task_module_request = cJSON_CreateArray();
    
    while (agent_id = agent_ids[agent++], agent_id != OS_INVALID) {
        if (task->message) {
            mtinfo(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_ACK_RECEIVED, agent_id, task->error_code, task->message);
        }
        // Send task update to task manager and bring back the response
        cJSON *request = wm_agent_upgrade_parse_task_module_request(WM_UPGRADE_AGENT_UPDATE_STATUS, agent_id, task->status);
        cJSON_AddItemToArray(json_task_module_request, request);        
    }    

    wm_agent_upgrade_task_module_callback(json_response, json_task_module_request, wm_agent_upgrade_update_status_success_callback, NULL);

    char *message = cJSON_PrintUnformatted(json_response);
    cJSON_Delete(json_task_module_request);
    cJSON_Delete(json_response);
    return message;
}

static cJSON* wm_agent_upgrade_analyze_agent(int agent_id, wm_agent_task *agent_task, wm_upgrade_error_code *error_code, const wm_manager_configs* manager_configs) {
    cJSON *task_request = NULL;

    // Agent information
    agent_task->agent_info = wm_agent_upgrade_init_agent_info();
    agent_task->agent_info->agent_id = agent_id;

    if (!wdb_agent_info(agent_id,
                        &agent_task->agent_info->platform,
                        &agent_task->agent_info->major_version,
                        &agent_task->agent_info->minor_version,
                        &agent_task->agent_info->architecture,
                        &agent_task->agent_info->wazuh_version,
                        &agent_task->agent_info->last_keep_alive)) {

        // Validate agent and task information
        *error_code = wm_agent_upgrade_validate_agent_task(agent_task, manager_configs);

        if (*error_code == WM_UPGRADE_SUCCESS) {
            // Save task entry for agent
            int result = wm_agent_upgrade_create_task_entry(agent_id, agent_task);

            if (result == OSHASH_SUCCESS) {
                task_request = wm_agent_upgrade_parse_task_module_request(agent_task->task_info->command, agent_id, NULL);
            } else if (result == OSHASH_DUPLICATED) {
                *error_code = WM_UPGRADE_UPGRADE_ALREADY_IN_PROGRESS;
            } else {
                *error_code = WM_UPGRADE_UNKNOWN_ERROR;
            }
        }
    } else {
        *error_code = WM_UPGRADE_GLOBAL_DB_FAILURE;
    }

    return task_request;
}

static int wm_agent_upgrade_validate_agent_task(const wm_agent_task *agent_task, const wm_manager_configs* manager_configs) {
    int validate_result = WM_UPGRADE_SUCCESS;
    cJSON *status_json = NULL;
    char *status = NULL;

    // Validate agent id
    validate_result = wm_agent_upgrade_validate_id(agent_task->agent_info->agent_id);

    if (validate_result != WM_UPGRADE_SUCCESS) {
        return validate_result;
    }

    // Validate agent status
    validate_result = wm_agent_upgrade_validate_status(agent_task->agent_info->last_keep_alive);

    if (validate_result != WM_UPGRADE_SUCCESS) {
        return validate_result;
    }

    // Validate if there is a task in progress for this agent
    status_json = wm_agent_upgrade_send_single_task(WM_UPGRADE_AGENT_GET_STATUS, agent_task->agent_info->agent_id, NULL);
    if (!wm_agent_upgrade_validate_task_status_message(status_json, &status, NULL)) {
        validate_result = WM_UPGRADE_TASK_MANAGER_COMMUNICATION;
    } else if (status && !strcmp(status, task_statuses[WM_TASK_IN_PROGRESS])) {
        validate_result = WM_UPGRADE_UPGRADE_ALREADY_IN_PROGRESS;
    }

    cJSON_Delete(status_json);
    os_free(status);

    if (validate_result != WM_UPGRADE_SUCCESS) {
        return validate_result;
    }

    // Validate Wazuh version to upgrade
    validate_result = wm_agent_upgrade_validate_version(agent_task->agent_info, agent_task->task_info->task, agent_task->task_info->command, manager_configs);

    if (validate_result != WM_UPGRADE_SUCCESS) {
        return validate_result;
    }

    // Validate WPK file
    if (WM_UPGRADE_UPGRADE == agent_task->task_info->command) {
        validate_result = wm_agent_upgrade_validate_wpk((wm_upgrade_task *)agent_task->task_info->task);
    } else {
        validate_result = wm_agent_upgrade_validate_wpk_custom((wm_upgrade_custom_task *)agent_task->task_info->task);
    }

    return validate_result;
}

static void wm_agent_upgrade_start_upgrades(cJSON *json_response, const cJSON* task_module_request, const wm_manager_configs* manager_configs) {
    unsigned int index = 0;
    OSHashNode *node = NULL;
    char *agent_key = NULL;
    wm_agent_task *agent_task = NULL;

    // Send request to task module and store task ids
    if (!wm_agent_upgrade_task_module_callback(json_response, task_module_request, wm_agent_upgrade_upgrade_success_callback, wm_agent_upgrade_remove_entry)) {
        node = wm_agent_upgrade_get_first_node(&index);

        while (node) {
            cJSON *status_json = NULL;
            agent_key = node->key;
            int agent_id;
            agent_task = (wm_agent_task *)node->data;
            node = wm_agent_upgrade_get_next_node(&index, node);

            if (!wm_agent_upgrade_send_wpk_to_agent(agent_task, manager_configs)) {

                if (WM_UPGRADE_UPGRADE == agent_task->task_info->command) {
                    wm_upgrade_task *upgrade_task = agent_task->task_info->task;
                    if (upgrade_task->custom_version && (wm_agent_upgrade_compare_versions(upgrade_task->custom_version, WM_UPGRADE_NEW_UPGRADE_MECHANISM) < 0)) {
                        // Update task to "Legacy". The agent won't report the result of the upgrade task
                        status_json = wm_agent_upgrade_send_single_task(WM_UPGRADE_AGENT_UPDATE_STATUS, agent_task->agent_info->agent_id, task_statuses[WM_TASK_LEGACY]);
                    } else {
                        // Update task to "In progress"
                        status_json = wm_agent_upgrade_send_single_task(WM_UPGRADE_AGENT_UPDATE_STATUS, agent_task->agent_info->agent_id, task_statuses[WM_TASK_IN_PROGRESS]);
                    }
                } else {
                    // Update task to "In progress". There is no way to know if this task should be legacy
                    status_json = wm_agent_upgrade_send_single_task(WM_UPGRADE_AGENT_UPDATE_STATUS, agent_task->agent_info->agent_id, task_statuses[WM_TASK_IN_PROGRESS]);
                }
            } else {
                // Update task to "Failed"
                status_json = wm_agent_upgrade_send_single_task(WM_UPGRADE_AGENT_UPDATE_STATUS, agent_task->agent_info->agent_id, task_statuses[WM_TASK_FAILED]);
            }

            wm_agent_upgrade_validate_task_status_message(status_json, NULL, &agent_id);

            wm_agent_upgrade_remove_entry(atoi(agent_key));

            cJSON_Delete(status_json);
        }
    } else {
        mtwarn(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_NO_AGENTS_TO_UPGRADE);
    }
}

static int wm_agent_upgrade_send_wpk_to_agent(const wm_agent_task *agent_task, const wm_manager_configs* manager_configs) {
    int result = OS_INVALID;
    char *file_path = NULL;
    char *file_sha1 = NULL;
    char *wpk_path = NULL;
    char *installer = NULL;

    mtdebug1(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_SENDING_WPK_TO_AGENT, agent_task->agent_info->agent_id);

    if (WM_UPGRADE_UPGRADE == agent_task->task_info->command) {
        wm_upgrade_task *upgrade_task = NULL;
        upgrade_task = agent_task->task_info->task;
        // WPK file path
        os_calloc(OS_SIZE_4096, sizeof(char), file_path);
        snprintf(file_path, OS_SIZE_4096, "%s%s", WM_UPGRADE_WPK_DEFAULT_PATH, upgrade_task->wpk_file);
        // WPK file sha1
        os_strdup(upgrade_task->wpk_sha1, file_sha1);
    } else {
        wm_upgrade_custom_task *upgrade_custom_task = NULL;
        upgrade_custom_task = agent_task->task_info->task;
        // WPK custom file path
        os_strdup(upgrade_custom_task->custom_file_path, file_path);
        // WPK custom file sha1
        os_calloc(41, sizeof(char), file_sha1);
        OS_SHA1_File(file_path, file_sha1, OS_BINARY);
        // Installer
        if (upgrade_custom_task->custom_installer) {
            os_strdup(upgrade_custom_task->custom_installer, installer);
        }
    }

    if (!installer) {
        if (!strcmp(agent_task->agent_info->platform, "windows")) {
            os_strdup("upgrade.bat", installer);
        } else {
            os_strdup("upgrade.sh", installer);
        }
    }

    wpk_path = basename_ex(file_path);

    // lock_restart
    if (!wm_agent_upgrade_send_lock_restart(agent_task->agent_info->agent_id)) {

        // open wb
        if (!wm_agent_upgrade_send_open(agent_task->agent_info->agent_id, wpk_path)) {

            // write
            if (!wm_agent_upgrade_send_write(agent_task->agent_info->agent_id, wpk_path, file_path, manager_configs->chunk_size)) {

                // close
                if (!wm_agent_upgrade_send_close(agent_task->agent_info->agent_id, wpk_path)) {

                    // sha1
                    result = wm_agent_upgrade_send_sha1(agent_task->agent_info->agent_id, wpk_path, file_sha1);
                }
            }
        }
    }

    if (!result) {
        result = wm_agent_upgrade_send_upgrade(agent_task->agent_info->agent_id, wpk_path, installer);
    }

    os_free(file_path);
    os_free(file_sha1);
    os_free(installer);

    return result;
}

static int wm_agent_upgrade_send_lock_restart(int agent_id) {
    int result = OS_INVALID;
    char *command = NULL;
    char *response = NULL;
    char *data = NULL;

    os_calloc(OS_MAXSTR, sizeof(char), command);

    snprintf(command, OS_MAXSTR, "%.3d com lock_restart -1",agent_id);

    response = wm_agent_upgrade_send_command_to_agent(command, strlen(command));

    result = wm_agent_upgrade_parse_agent_response(response, &data);
    
    os_free(command);
    os_free(response);

    return result;
}

static int wm_agent_upgrade_send_open(int agent_id, const char *wpk_file) {
    int result = OS_INVALID;
    char *command = NULL;
    char *response = NULL;
    char *data = NULL;
    int open_retries = 0;

    os_calloc(OS_MAXSTR, sizeof(char), command);

    snprintf(command, OS_MAXSTR, "%.3d com open wb %s", agent_id, wpk_file);

    for (open_retries = 0; open_retries < WM_UPGRADE_WPK_OPEN_ATTEMPTS; ++open_retries) {
        os_free(response);
        response = wm_agent_upgrade_send_command_to_agent(command, strlen(command));
        if (result = wm_agent_upgrade_parse_agent_response(response, &data), !result) {
            break;
        }
    }

    os_free(command);
    os_free(response);

    return result;
}

static int wm_agent_upgrade_send_write(int agent_id, const char *wpk_file, const char *file_path, int chunk_size) {
    int result = OS_INVALID;
    char *command = NULL;
    char *response = NULL;
    char *data = NULL;
    FILE *file = NULL;
    unsigned char buffer[chunk_size];
    size_t bytes = 0;
    size_t command_size = 0;
    size_t byte = 0;

    os_calloc(OS_MAXSTR, sizeof(char), command);

    if (file = fopen(file_path, "rb"), file) {
        while (bytes = fread(buffer, 1, sizeof(buffer), file), bytes) {
            snprintf(command, OS_MAXSTR, "%.3d com write %ld %s ", agent_id, bytes, wpk_file);
            command_size = strlen(command);
            for (byte = 0; byte < bytes; ++byte) {
                sprintf(&command[command_size++], "%c", buffer[byte]);
            }
            os_free(response);
            response = wm_agent_upgrade_send_command_to_agent(command, command_size);
            if (result = wm_agent_upgrade_parse_agent_response(response, &data), result) {
                break;
            }
        }
        fclose(file);
    }

    os_free(command);
    os_free(response);

    return result;
}

static int wm_agent_upgrade_send_close(int agent_id, const char *wpk_file) {
    int result = OS_INVALID;
    char *command = NULL;
    char *response = NULL;
    char *data = NULL;

    os_calloc(OS_MAXSTR, sizeof(char), command);

    snprintf(command, OS_MAXSTR, "%.3d com close %s", agent_id, wpk_file);

    response = wm_agent_upgrade_send_command_to_agent(command, strlen(command));

    result = wm_agent_upgrade_parse_agent_response(response, &data);
    
    os_free(command);
    os_free(response);

    return result;
}

static int wm_agent_upgrade_send_sha1(int agent_id, const char *wpk_file, const char *file_sha1) {
    int result = OS_INVALID;
    char *command = NULL;
    char *response = NULL;
    char *data = NULL;

    os_calloc(OS_MAXSTR, sizeof(char), command);

    snprintf(command, OS_MAXSTR, "%.3d com sha1 %s", agent_id, wpk_file);

    response = wm_agent_upgrade_send_command_to_agent(command, strlen(command));

    if (result = wm_agent_upgrade_parse_agent_response(response, &data), !result) {
        if (strcmp(file_sha1, data)) {
            mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_AGENT_RESPONSE_SHA1_ERROR);
            result = OS_INVALID;
        }
    }

    os_free(command);
    os_free(response);

    return result;
}

static int wm_agent_upgrade_send_upgrade(int agent_id, const char *wpk_file, const char *installer) {
    int result = OS_INVALID;
    char *command = NULL;
    char *response = NULL;
    char *data = NULL;

    os_calloc(OS_MAXSTR, sizeof(char), command);

    snprintf(command, OS_MAXSTR, "%.3d com upgrade %s %s", agent_id, wpk_file, installer);

    response = wm_agent_upgrade_send_command_to_agent(command, strlen(command));
    result = wm_agent_upgrade_parse_agent_response(response, &data);
    
    os_free(command);
    os_free(response);

    return result;
}

char* wm_agent_upgrade_send_command_to_agent(const char *command, const size_t command_size) {
    char *response = NULL;
    int length = 0;

    const char *path = isChroot() ? REMOTE_REQ_SOCK : DEFAULTDIR REMOTE_REQ_SOCK;

    int sock = OS_ConnectUnixDomain(path, SOCK_STREAM, OS_MAXSTR);

    if (sock == OS_SOCKTERR) {
        mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_UNREACHEABLE_REQUEST, path);
    } else {
        mtdebug2(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_REQUEST_SEND_MESSAGE, command);

        OS_SendSecureTCP(sock, command_size, command);
        os_calloc(OS_MAXSTR, sizeof(char), response);

        switch (length = OS_RecvSecureTCP(sock, response, OS_MAXSTR), length) {
            case OS_SOCKTERR:
                mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_SOCKTERR_ERROR);
                break;
            case -1:
                mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_RECV_ERROR, strerror(errno));
                break;
            default:
                if (!response) {
                    mterror(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_EMPTY_AGENT_RESPONSE);
                } else {
                    mtdebug2(WM_AGENT_UPGRADE_LOGTAG, WM_UPGRADE_REQUEST_RECEIVE_MESSAGE, response);
                }
                break;
        }

        close(sock);
    }

    return response;
}

static cJSON* wm_agent_upgrade_send_single_task(wm_upgrade_command command, int agent_id, const char* status_task) {
    cJSON *message_object = wm_agent_upgrade_parse_task_module_request(command, agent_id, status_task);
    cJSON *message_array = cJSON_CreateArray();
    cJSON_AddItemToArray(message_array, message_object);

    cJSON* task_module_response = cJSON_CreateArray();
    wm_agent_upgrade_task_module_callback(task_module_response, message_array, NULL, NULL);

    cJSON_Delete(message_array);

    cJSON* response = cJSON_DetachItemFromArray(task_module_response, 0);
    cJSON_Delete(task_module_response);
    return response;
}
