/*
 * Wazuh Shared Configuration Manager
 * Copyright (C) 2015-2020, Wazuh Inc.
 * April 3, 2018.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef SHARED_DOWNLOAD_H
#define SHARED_DOWNLOAD_H

#include <external/libyaml/include/yaml.h>
#include <pthread.h>
#include "shared.h"

#define W_PARSER_ERROR -1
#define W_SHARED_YAML_FILE "files.yml"
#define W_PARSER_STARTED "Started yaml parsing of file: %s"
#define W_PARSER_SUCCESS "Successfully parsed of yaml file: %s"
#define W_PARSER_FAILED "Failed yaml parsing of file: %s"
#define W_PARSER_ERROR_INIT "Initializing yaml parser"
#define W_PARSER_ERROR_FILE "File %s not found"
#define W_PARSER_HASH_TABLE_ERROR "Creating OSHash"
#define W_PARSER_POLL "Wrong poll value: %s."
#define W_PARSER_FILE_CHANGED "File '%s' changed. Reloading data"
#define W_PARSER_GROUP_TOO_LARGE "The group name is too large. The maximum length is %d"

/**
 * Structure to represent group's files.
 */
typedef struct _sd_file {
    char *name; /**< File's name. */
    char *url;  /**< File's url. */
} sd_file_t;

/**
 * Structure to represent configuration's groups.
 */
typedef struct _sd_group {
    char *name;                 /**< Group's name. */
    sd_file_t *files;           /**< Pointer to sd_file structure. */
    int n_files;                /**< Number of files of each group. */
    int poll_download_rate;     /**< Download rate in seconds of the specified files. */
    int current_polling_time;   /**< Current time to reload files */
    int merge_file_index;       /**< Index of merged.md file. */
    int merged_is_downloaded;   /**< Check if merged.mg file is downloaded. */
    
} sd_group_t;

/**
 * Structure to represent YAML nodes.
 */
typedef struct _sd_yaml_node {
    yaml_node_t *key;           /**< Mapping YAML node key. */
    yaml_node_t *value;         /**< Mapping YAML node value. */
    yaml_node_pair_t *pair_i;   /**< YAML node pair key and value. */
    char *scalar;               /**< YAML node's value. */
} sd_yaml_node;

/**
 * Structure to represent configuration's agents.
 */
typedef struct _sd_agent {
    char *name;     /**< Agent's name. */
    char *group;    /**< Agent's group name. */
} sd_agent_t;

/**
 * Structure to represent Shared Download configuration.
 */
typedef struct _sd_config {
    int n_agents;                       /**< Agents' number. */
    sd_agent_t *agents;                 /**< Pointer to sd_agent_t structure. */
    int n_groups;                       /**< Group's number. */
    sd_group_t *groups;                 /**< Pointer to sd_group_t structure. */
    char file_name[OS_SIZE_1024 + 1];   /**< YAML file's name. */
    time_t file_date;                   /**< Controll files date modifications. */
    ino_t file_inode;                   /** File serial number. **/
    OSHash *ptable;                     /**< Pointer to ptable structure. */
    pthread_mutex_t mutex;              /**< Thread lock. */
    int checked_url_connection;         /**< Check URL connection. */
} sd_config_t;

/** 
 * @brief It initializes the struct's configuration, allocates
 * memory and creates the Hash table and initializes the lock.
 * 
 * @param config The shared download configuration.
 * 
 * @returns @c 0 if the Hash table creatin wasn't successful and
 * @c 1 in other case.
 */
int sd_init(sd_config_t **config);

/**
 * @brief We get a pointer to the sd_group_t structure.
 * 
 * @param config The shared download configuration.
 * 
 * @param name The group's name.
 * 
 * @returns A pointer to the sd_group_t structure from our configuration.
 */
sd_group_t *sd_get_group(sd_config_t *config, const char *name);

/**
 * @brief We get a pointer to the sd_agent_t structure.
 * 
 * @param config The shared download configuration.
 * 
 * @param name The agent's name.
 * 
 * @returns A pointer to the sd_agent_t structure from our configuration.
 */
sd_agent_t *sd_get_agent(sd_config_t *config, const char *name);

/**
 * @brief Adds an agent to the HASH table.
 * 
 * @param config The shared download configuration.
 */
void sd_add_agent(sd_config_t *config);

/**
 * @brief Adds a group to the HASH table.
 * 
 * @param config The shared download configuration.
 */
void sd_add_group(sd_config_t *config);

/**
 * @brief Calls sd_create_directory for each group.
 * 
 * @param config The shared download configuration.
 */
void sd_create_groups_directory(sd_config_t *config);

/**
 * @brief Creates directories.
 * 
 * @param group The group's name.
 */
void sd_create_directory(char *group);

/**
 * @brief Gets YAML node value.
 * 
 * @param data YAML node.
 * 
 * @returns YAML node value.
 */
char *sd_get_scalar(yaml_node_t *data);

/**
 * @brief Parses YAML files.
 * 
 * @param document YAML document.
 * @param root_node YAML root node.
 * @param files The files.
 * @param n_files File's number.
 * 
 * @returns @c 1 if files' parse was successful and @c 0 in other case.
 */
int sd_parse_files(yaml_document_t * document, yaml_node_t *root_node, sd_file_t **files, int *n_files);

/**
 * @brief Parses YAML poll option.
 * 
 * @param root_node YAML root node.
 * @param group The group's poll.
 * 
 * @returns @c 1 if poll parse was successful and @c 0 in other case.
 */
int sd_parse_poll(yaml_node_t *root_node, sd_group_t *group);

/**
 * @brief Parses a single configuration's group. 
 * 
 * @param document YAML document.
 * @param root_node YAML root node.
 * @param group The group.
 * 
 * @returns @c 1 if the group parse was successful and @c 0 in other case.
 */
int sd_parse_group(yaml_document_t * document, yaml_node_t *root_node, sd_group_t *group);

/**
 * @brief Parses configuration's groups. 
 * 
 * @param document YAML document.
 * @param root_node YAML root node.
 * @param groups The groups.
 * @param n_groups Groups' number.
 * 
 * @returns @c 1 if groups' parse was successful and @c 0 in other case.
 */
int sd_parse_groups(yaml_document_t * document, yaml_node_t *root_node, sd_group_t **groups, int *n_groups);

/**
 * @brief Parses configuration's agents.
 * 
 * @param document YAML document.
 * @param root_node YAML root node.
 * @param groups The agents.
 * @param n_groups agents' number.
 * 
 * @returns @c 1 if agents' parse was successful and @c 0 in other case.
 */
int sd_parse_agents(yaml_document_t *document, yaml_node_t *root_node, sd_agent_t **agents, int *n_agents);

/**
 * @brief Parses configuration.
 * 
 * @param config The shared download configuration.
 * 
 * @returns @c 1 if configuration's parse was successful and @c 0 in other case.
 */
int sd_parse(sd_config_t **config);

/**
 * @brief Reload document's date and inode, calls sd_parse function,
 * sd_add_group, sd_add_agent.
 * 
 * @returns @c 1 if configuration's load was successful and @c 0 in other case.
 */
int sd_load(sd_config_t **config/*, const char *filepath*/);

/**
 * @brief It deallocates the memory previously allocated. 
 * 
 * @param config The shared download configuration.
 */
void sd_destroy_content(sd_config_t **config);

/**
 * @brief It reloads the file's configuration
 * in case it was changed and the parse was
 * successfully. 
 * 
 * @param config The shared download configuration.
 * 
 * @returns @c 1 in case it was successfully, 
 * @c 0 in other case.
 */
int sd_reload(sd_config_t **config);

/**
 * @brief It checks if the yaml file has changed.
 * 
 * @param config The shared download configuration.
 *
 * @returns @c 1 if the file has changed or @c 0 in other case.
 */
int sd_file_changed(sd_config_t *config);

/**
 * @brief Check download module connection
 */
void check_download_module_connection();

#endif /* SHARED_DOWNLOAD_H */
