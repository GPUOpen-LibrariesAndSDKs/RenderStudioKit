# Copyright 2023 Advanced Micro Devices, Inc
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import subprocess
from pathlib import Path
import httpx
import traceback
import asyncio
import json
import os
import sys

from app.logger import logger, syncthing_logger
from app.settings import settings
from app.terminator import terminator

if sys.platform == "win32":
    asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())

class SyncthingManager:
    def __init__(self, exe_path: str, workspace_path: str):
        self.process = None

        if sys.platform == "win32":
            self.exe_path = f"{exe_path}.exe"
        else:
            self.exe_path = exe_path

        self.workspace_path = Path(workspace_path)
        self.max_event_id = 0

    async def communicate(self):
        async for line in self.process.stdout:
            if line:
                syncthing_logger.info(line.decode().strip())

    async def start(self):
        if getattr(sys, 'frozen', False):
            application_path = os.path.dirname(sys.executable)
        elif __file__:
            application_path = os.path.dirname(__file__)

        home = self.workspace_path.parent / 'Syncthing'
        logger.info(f'[Syncthing] Launching {self.exe_path} with home: {home}')

        try:
            self.process = await asyncio.create_subprocess_exec(
                os.path.join(application_path, self.exe_path),
                f"--home",
                str(home),
                "--no-default-folder",
                "--skip-port-probing",
                f"--gui-address={settings.SYNCTHING_URL}",
                f"--gui-apikey={settings.SYNCTHING_API_KEY}",
                "--no-browser",
                "--no-restart",
                "--no-upgrade",
                "--log-max-size=0",
                "--log-max-old-files=0",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
        except Exception:
            logger.error(f"[Syncthing] Fatal error on syncthing startup: {traceback.format_exc()}")
            await asyncio.create_task(terminator.terminate_all())

        asyncio.create_task(self.communicate())

        async with httpx.AsyncClient() as client:
            while True:
                try:
                    r = await client.get(f"{settings.SYNCTHING_URL}/rest/noauth/health")
                    if r.status_code == httpx.codes.OK:
                        break
                except:
                    pass

        try:
            async with httpx.AsyncClient() as client:
                config = (await client.get(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                local_id = (await client.get(f"{settings.SYNCTHING_URL}/rest/system/status", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()['myID']

                if settings.REMOTE_URL != 'localhost':
                    remote = (await client.get(f"{settings.REMOTE_URL}/workspace/info", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                else:
                    remote = {
                        'folder_id': 'main-rs-workspace',
                        'folder_name': 'AMD RenderStudio Workspace',
                        'device_id': local_id,
                        'device_name': "AMD RenderStudio Syncthing Server"
                    }

                # Setup folder
                existing_folder = next((folder for folder in config['folders'] if folder['id'] == remote['folder_id']), None)
                if not existing_folder:
                    folder = (await client.get(f"{settings.SYNCTHING_URL}/rest/config/defaults/folder", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                    folder['id'] = remote['folder_id']
                    folder['label'] = remote['folder_name']
                    folder['path'] = settings.WORKSPACE_DIR
                    folder['devices'].append({
                        'deviceID': remote['device_id'],
                        'introducedBy': '',
                        'encryptionPassword': ''
                    })
                    folder['fsWatcherDelayS'] = 1
                    config['folders'].append(folder)
                else:
                    existing_folder['id'] = remote['folder_id']
                    existing_folder['label'] = remote['folder_name']
                    existing_folder['path'] = settings.WORKSPACE_DIR
                    existing_folder['devices'].append({
                        'deviceID': remote['device_id'],
                        'introducedBy': '',
                        'encryptionPassword': ''
                    })
                    existing_folder['fsWatcherDelayS'] = 1

                # Setup device
                existing_device = next((device for device in config['devices'] if device['deviceID'] == remote['device_id']), None)
                if not existing_device:
                    device = (await client.get(f"{settings.SYNCTHING_URL}/rest/config/defaults/device", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                    device['deviceID'] = remote['device_id']
                    device['name'] = remote['device_name']
                    device['addresses'].append(settings.REMOTE_URL.replace('http:/', 'tcp:/').replace('https:/', 'tcp:/') + ":22000")
                    config['devices'].append(device)
                else:
                    existing_device['deviceID'] = remote['device_id']
                    existing_device['name'] = remote['device_name']
                    existing_device['addresses'].append(settings.REMOTE_URL.replace('http:/', 'tcp:/').replace('https:/', 'tcp:/') + ":22000")

                # Optimizations
                config['options']['relaysEnabled'] = False
                if settings.REMOTE_URL == 'localhost':
                    config['options']['setLowPriority'] = False

                response = await client.put(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}, data=json.dumps(config))
                logger.info(f"[Syncthing] Local config update: {response.status_code==200}")

        except Exception:
            logger.error(f"[Syncthing] Fatal error on syncthing startup: {traceback.format_exc()}")
            await asyncio.create_task(terminator.terminate_all())

        try:
            if settings.REMOTE_URL != 'localhost':
                if await self.has_folder_errors():
                    await self.fix_folder_errors()
        except Exception:
            print(f"[Syncthing] Can't fix folder errors: {traceback.format_exc()}")

    async def get_events(self):
        try:
            async with httpx.AsyncClient(timeout=120) as client:
                logger.info(f'[Syncthing] Polling events with id {self.max_event_id}')
                response = await client.get(f"{settings.SYNCTHING_URL}/rest/events?since={self.max_event_id}&timeout=120", headers={'Authorization': F'Bearer {settings.SYNCTHING_API_KEY}'})
                if response.status_code == 200:
                    if response.content:
                        events = response.json()
                    else:
                        logger.error(f"[Syncthing] Got empty response with status {response.status_code}")
                        return []
                else:
                    logger.error(f"[Syncthing] Got response with status {response.status_code}: {response.content}")
                    return []
        except Exception:
            logger.error(f"[Syncthing] get_events error: {traceback.format_exc()}")
            return []
        first_poll = self.max_event_id == 0
        for event in events:
            self.max_event_id = max(self.max_event_id, event['id'])
        return [] if first_poll else events

    async def get_device(self):
        async with httpx.AsyncClient() as client:
            config = (await client.get(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
            local_id = (await client.get(f"{settings.SYNCTHING_URL}/rest/system/status", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()['myID']
            existing_device = next((device for device in config['devices'] if device['deviceID'] == local_id), None)
            if not existing_device:
                logger.error("[Syncthing] Can't find local device")
            return existing_device

    async def get_config(self):
        async with httpx.AsyncClient() as client:
            return (await client.get(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()

    async def put_config(self, config):
        async with httpx.AsyncClient() as client:
            return await client.put(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}, data=json.dumps(config))

    async def cleanup_devices(self, client):
        try:
            connections = (await client.get(f"{settings.SYNCTHING_URL}/rest/system/connections", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
            device_id_to_remove = []

            for device_id in connections['connections'].keys():
                if not connections['connections'][device_id]['connected']:
                    device_id_to_remove.append(device_id)

            for device_id in device_id_to_remove:
                await self.remove_device(device_id)

            logger.info(f"[Syncthing] Cleanup removed {len(device_id_to_remove)} devices!")
        except Exception:
            logger.error(f"[Syncthing] Can't cleanup devices: {traceback.format_exc()}")

    async def append_device(self, device):
        try:
            logger.info(f"[Syncthing] Request to add device")
            config = await self.get_config()
            config['devices'].append(device)
            response = await self.put_config(config)
            logger.info(f"[Syncthing] Add device status: {response.status_code==200}")
        except Exception:
            logger.error(f"[Syncthing] Can't add device: {traceback.format_exc()}")

    async def remove_device(self, device_id):
        try:
            logger.info(f"[Syncthing] Request to remove device")
            config = await self.get_config()

            existing_device = next((device for device in config['devices'] if device['deviceID'] == device_id), None)
            if existing_device:
                logger.info('Found device in devices')
                config['devices'].remove(existing_device)

            existing_folder_device = next((device for device in config['folders'][0]['devices'] if device['deviceID'] == device_id), None)
            if existing_folder_device:
                logger.info('Found device in folders')
                config['folders'][0]['devices'].remove(existing_folder_device)

            response = await self.put_config(config)
            logger.info(f"[Syncthing] Remove device status: {response.status_code==200}")
        except Exception:
            logger.error(f"[Syncthing] Can't remove device: {traceback.format_exc()}")

    async def append_folder(self, folder):
        try:
            logger.info("[Syncthing] Request to share folder")
            config = await self.get_config()
            it = next((it for it in config['folders'] if it['id'] == folder['folderID']), None)
            if not it:
                logger.error("[Syncthing] Trying to add non existent folder")
                return
            it['devices'].append({'deviceID': folder['deviceID']})
            response = await self.put_config(config)
            logger.info(f"[Syncthing] Share folder status: {response.status_code==200}")
        except Exception:
            logger.error(f"[Syncthing] Can't share folder: {traceback.format_exc()}")

    async def fix_folder_errors(self):
        try:
            logger.info("[Syncthing] Trying to fix shared folder")

            if settings.REMOTE_URL == 'localhost':
                logger.error("[Syncthing] Trying to fix folder when you are server is disabled")
                return

            config = await self.get_config()

            async with httpx.AsyncClient() as client:
                local_id = (await client.get(f"{settings.SYNCTHING_URL}/rest/system/status", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()['myID']
                remote = (await client.get(f"{settings.REMOTE_URL}/workspace/info", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()

                it = next((it for it in config['folders'] if it['id'] == remote['folder_id']), None)
                if not it:
                    logger.error("[Syncthing] Trying to fix non existent folder")
                    return

                it['devices'].clear()
                it['devices'].append({'deviceID': local_id})
                response = await self.put_config(config)
                logger.info(f"[Syncthing] Fixing, clear device status: {response.status_code==200}")

                it['devices'].append({'deviceID': remote['device_id']})
                response = await self.put_config(config)
                logger.info(f"[Syncthing] Fixing, append again device status: {response.status_code==200}")
        except Exception:
            logger.error(f"[Syncthing] Can't fix folder errors: {traceback.format_exc()}")

    async def has_folder_errors(self):
        try:
            async with httpx.AsyncClient() as client:
                response = (await client.get(f"{settings.SYNCTHING_URL}/rest/folder/errors?folder=main-rs-workspace", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                if response['errors']:
                    return True
                else:
                    return False
        except Exception:
            logger.error(f"[Syncthing] Caught exception in has_folder_errors getter: {traceback.format_exc()}")
            return False

    async def is_connected(self):
        try:
            async with httpx.AsyncClient() as client:
                response = (await client.get(f"{settings.SYNCTHING_URL}/rest/system/connections", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                remote = (await client.get(f"{settings.REMOTE_URL}/workspace/info", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                return response['connections'][remote['device_id']]['connected']
        except Exception:
            logger.error(f"[Syncthing] Caught exception in is_connected getter: {traceback.format_exc()}")
            return False

    async def pause(self):
        try:
            config = await self.get_config()
            config['folders'][0]['paused'] = True
            response = await self.put_config(config)
        except Exception:
            logger.error(f"[Syncthing] Caught exception in pause: {traceback.format_exc()}")

    async def resume(self):
        try:
            config = await self.get_config()
            config['folders'][0]['paused'] = False
            response = await self.put_config(config)
        except Exception:
            logger.error(f"[Syncthing] Caught exception in resume: {traceback.format_exc()}")

    async def terminate(self):
        try:
            async with httpx.AsyncClient() as client:
                result = await client.post(f"{settings.SYNCTHING_URL}/rest/system/shutdown", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})
        except Exception:
            logger.error(f"[Terminate Syncthing] Rest request to shutdown: {traceback.format_exc()} (that should be fine)")

        try:
            await self.process.wait()
        except Exception:
            logger.error(f"[Terminate Syncthing] Waiting for working process exit: {traceback.format_exc()} (that should be fine)")

        self.max_event_id = 0

syncthing_manager = SyncthingManager("syncthing", settings.WORKSPACE_DIR)
