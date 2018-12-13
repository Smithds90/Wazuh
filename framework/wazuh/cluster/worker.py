# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2
import asyncio
import errno
import functools
import operator
import os
import shutil
import time
from typing import Tuple, Dict, Callable
from wazuh.cluster import client, cluster, common as c_common
from wazuh import cluster as metadata
from wazuh import common
from wazuh.cluster.dapi import dapi


class ReceiveIntegrityTask(c_common.ReceiveFileTask):

    def set_up_coro(self) -> Callable:
        return self.wazuh_common.process_files_from_master


class SyncWorker:
    """
    Defines methods to synchronize files with master
    """
    def __init__(self, cmd: bytes, files_to_sync: Dict, checksums: Dict, logger, worker):
        self.cmd = cmd
        self.files_to_sync = files_to_sync
        self.checksums = checksums
        self.logger = logger
        self.worker = worker

    async def sync(self):
        result = await self.worker.send_request(command=self.cmd+b'_p', data=b'')
        if result.startswith(b'Error'):
            self.logger.error(b'Error asking for permission: ' + result)
            return
        elif result == b'False':
            self.logger.info('Master didnt grant permission to synchronize')
            return
        else:
            self.logger.info("Permission to synchronize granted.")

        self.logger.info("Compressing files")
        compressed_data_path = cluster.compress_files(name=self.worker.name, list_path=self.files_to_sync,
                                                      cluster_control_json=self.checksums)
        task_id = await self.worker.send_request(command=self.cmd, data=b'')

        self.logger.info("Sending compressed file to master")
        result = await self.worker.send_file(filename=compressed_data_path)
        if result.startswith(b'Error'):
            self.logger.error(b"Error sending files information: " + result)
            return
        else:
            self.logger.info("Worker files sent to master.")

        result = await self.worker.send_request(command=self.cmd+b'_e', data=task_id + b' ' + compressed_data_path.encode())
        if result.startswith(b'Error'):
            self.logger.error(result)


class WorkerHandler(client.AbstractClient, c_common.WazuhCommon):

    def __init__(self, version, node_type, cluster_name, **kwargs):
        super().__init__(**kwargs, tag="Worker")
        self.client_data = "{} {} {} {}".format(self.name, cluster_name, node_type, version).encode()

    def process_request(self, command: bytes, data: bytes) -> Tuple[bytes, bytes]:
        self.logger.debug("Command received: '{}'".format(command))
        if command == b'sync_m_c_ok':
            return b'ok', b'Thanks'
        elif command == b'sync_m_c':
            return self.setup_receive_files_from_master()
        elif command == b'sync_m_c_e':
            return self.end_receiving_integrity(data.decode())
        elif command == b'dapi_res':
            client, result = data.split(b' ', 1)
            asyncio.create_task(self.manager.local_server.clients[client.decode()].send_request(b'dapi_res', result))
            return b'ok', b'Response forwarded to worker'
        elif command == b'dapi':
            self.manager.dapi.add_request(b'None*' + data)
            return b'ok', b'Added request to API requests queue'
        else:
            return super().process_request(command, data)

    def setup_receive_files_from_master(self):
        return super().setup_receive_file(ReceiveIntegrityTask)

    def end_receiving_integrity(self, task_and_file_names: str) -> Tuple[bytes, bytes]:
        return super().end_receiving_file(task_and_file_names)

    async def sync_integrity(self):
        integrity_logger = self.setup_task_logger("Integrity")
        while True:
            if self.connected:
                before = time.time()
                await SyncWorker(cmd=b'sync_i_w_m', files_to_sync={}, checksums=cluster.get_files_status('master',
                                                                                                         self.name),
                                 logger=integrity_logger, worker=self).sync()
                after = time.time()
                integrity_logger.debug("Time synchronizing integrity: {} s".format(after - before))
            await asyncio.sleep(self.cluster_items['intervals']['worker']['sync_integrity'])

    async def sync_agent_info(self):
        agent_info_logger = self.setup_task_logger("Agent info")
        while True:
            if self.connected:
                before = time.time()
                agent_info_logger.info("Starting to send agent status files")
                worker_files = cluster.get_files_status('worker', self.name, get_md5=False)
                await SyncWorker(cmd=b'sync_a_w_m', files_to_sync=worker_files, checksums=worker_files,
                                 logger=agent_info_logger, worker=self).sync()
                after = time.time()
                agent_info_logger.debug2("Time synchronizing agent statuses: {} s".format(after - before))
            await asyncio.sleep(self.cluster_items['intervals']['worker']['sync_files'])

    async def sync_extra_valid(self, extra_valid: Dict):
        extra_valid_logger = self.setup_task_logger("Extra valid")
        before = time.time()
        self.logger.debug("Starting to send extra valid files")
        # TODO: Add support for more extra valid file types if ever added
        n_files, merged_file = cluster.merge_agent_info(merge_type='agent-groups', files=extra_valid.keys(),
                                                        time_limit_seconds=0, node_name=self.name)
        if n_files:
            files_to_sync = {merged_file: {'merged': True, 'merge_type': 'agent-groups', 'merge_name': merged_file,
                                           'cluster_item_key': '/queue/agent-groups/'}}
            my_worker = SyncWorker(cmd=b'sync_e_w_m', files_to_sync=files_to_sync, checksums=files_to_sync,
                                   logger=extra_valid_logger, worker=self)
            await my_worker.sync()
        after = time.time()
        self.logger.debug2("Time synchronizing extra valid files: {} s".format(after - before))

    async def process_files_from_master(self, name: str, file_received: asyncio.Event):
        await file_received.wait()
        self.logger.info("Analyzing received files: Start.")

        ko_files, zip_path = cluster.decompress_files(self.sync_tasks[name].filename)
        self.logger.info("Analyzing received files: Missing: {}. Shared: {}. Extra: {}. ExtraValid: {}".format(
            len(ko_files['missing']), len(ko_files['shared']), len(ko_files['extra']), len(ko_files['extra_valid'])))

        # Update files
        if ko_files['extra_valid']:
            self.logger.info("Master requires some worker files.")
            asyncio.create_task(self.sync_extra_valid(ko_files['extra_valid']))

        if not ko_files['shared'] and not ko_files['missing'] and not ko_files['extra']:
            self.logger.info("Worker meets integrity checks. No actions.")
        else:
            self.logger.info("Worker does not meet integrity checks. Actions required.")
            self.logger.info("Updating files: Start.")
            self.update_master_files_in_worker(ko_files, zip_path)
            self.logger.info("Updating files: End.")

    def update_master_files_in_worker(self, ko_files: Dict, zip_path: str):
        def overwrite_or_create_files(filename, data, content=None):
            # Cluster items information: write mode and umask
            cluster_item_key = data['cluster_item_key']
            w_mode = cluster_items[cluster_item_key]['write_mode']
            umask = cluster_items[cluster_item_key]['umask']

            if content is None:
                # Full path
                my_zip_path = "{}/{}".format(zip_path, filename)
                # File content and time
                with open(my_zip_path, 'rb') as f:
                    file_data = f.read()
            else:
                file_data = content

            tmp_path = '/queue/cluster/tmp_files'

            cluster._update_file(file_path=filename, new_content=file_data, umask_int=umask, w_mode=w_mode,
                                 tmp_dir=tmp_path, whoami='worker')

        cluster_items, errors = cluster.get_cluster_items()['files'], {'shared': 0, 'missing': 0, 'extra': 0}
        for filetype, files in ko_files.items():
            if filetype == 'shared' or filetype == 'missing':
                self.logger.debug("Received {} {} files to update from master.".format(len(ko_files[filetype]),
                                                                                       filetype))
                for filename, data in files.items():
                    try:
                        self.logger.debug2("Processing file {}".format(filename))
                        if data['merged']:
                            for name, content, _ in cluster.unmerge_agent_info('agent-groups', zip_path,
                                                                               filename):
                                overwrite_or_create_files(name, data, content)
                        else:
                            overwrite_or_create_files(filename, data)
                    except Exception as e:
                        errors[filetype] += 1
                        self.logger.error("Error processing {} file '{}': {}".format(filetype, filename, e))
                        continue
            elif filetype == 'extra':
                for file_to_remove in files:
                    try:
                        self.logger.debug2("Remove file: '{}'".format(file_to_remove))
                        file_path = common.ossec_path + file_to_remove
                        try:
                            os.remove(file_path)
                        except OSError as e:
                            if e.errno == errno.ENOENT and '/queue/agent-groups/' in file_path:
                                self.logger.debug2("File {} doesn't exist.".format(file_to_remove))
                                continue
                            else:
                                raise e
                    except Exception as e:
                        errors['extra'] += 1
                        self.logger.debug2("Error removing file '{}': {}".format(file_to_remove, str(e)))
                        continue

            directories_to_check = (os.path.dirname(f) for f, data in ko_files['extra'].items()
                                    if cluster_items[data['cluster_item_key']]['remove_subdirs_if_empty'])
            for directory in directories_to_check:
                try:
                    full_path = common.ossec_path + directory
                    dir_files = set(os.listdir(full_path))
                    if not dir_files or dir_files.issubset(set(cluster_items['excluded_files'])):
                        shutil.rmtree(full_path)
                except Exception as e:
                    errors['extra'] += 1
                    self.logger.debug2("Error removing directory '{}': {}".format(directory, str(e)))
                    continue

            if functools.reduce(operator.add, errors.values()) > 0:
                self.logger.error("Found errors: {} overwriting, {} creating and {} removing".format(errors['shared'],
                                                                                                     errors['missing'],
                                                                                                     errors['extra']))

    def get_logger(self, logger_tag: str = ''):
        return self.logger


class Worker(client.AbstractClientManager):

    def __init__(self, **kwargs):
        super().__init__(**kwargs, tag="Worker manager")
        self.cluster_name = self.configuration['name']
        self.version = metadata.__version__
        self.node_type = self.configuration['node_type']
        self.handler_class = WorkerHandler
        self.extra_args = {'cluster_name': self.cluster_name, 'version': self.version, 'node_type': self.node_type}
        self.dapi = dapi.APIRequestQueue(server=self)

    def add_tasks(self):
        return super().add_tasks() + [(self.client.sync_integrity, tuple()), (self.client.sync_agent_info, tuple()),
                                      (self.dapi.run, tuple())]
