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
import requests
import os
import sys

from app.logger import logger, syncthing_logger
from app.settings import settings

if sys.platform == "win32":
    asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())

class SyncthingManager:
    def __init__(self, exe_path: str, workspace_path: str):
        self.process = None
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

        logger.info(f'Launching {self.exe_path}')
        try:
            self.process = await asyncio.create_subprocess_exec(
                os.path.join(application_path, self.exe_path),
                f"--home={self.workspace_path.parent / 'Syncthing'}",
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
        except Exception as e:
            logger.error(traceback.format_exception(e))
            logger.info('Bye-bye')
            os._exit(1)

        asyncio.create_task(self.communicate())

        try:
            async with httpx.AsyncClient() as client:
                config = (await client.get(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                local_id = (await client.get(f"{settings.SYNCTHING_URL}/rest/system/status", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()['myID']

                if settings.REMOTE_URL != 'localhost':
                    remote = (await client.get(f"{settings.REMOTE_URL}/syncthing/info", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                else:
                    remote = {
                        'folder_id': 'main-rs-workspace',
                        'folder_name': 'AMD RenderStudio Workspace',
                        'device_id': local_id,
                        'device_name': "AND RenderStudio Syncthing Server"
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
                    config['folders'].append(folder)

                # Setup device
                existing_device = next((device for device in config['devices'] if device['deviceID'] == remote['device_id']), None)
                if not existing_device:
                    device = (await client.get(f"{settings.SYNCTHING_URL}/rest/config/defaults/device", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
                    device['deviceID'] = remote['device_id']
                    device['name'] = remote['device_name']
                    device['addresses'].append(settings.REMOTE_URL.replace('https:/', 'tcp:/') + '/syncthing:443')
                    config['devices'].append(device)
                else:
                    existing_device['name'] = remote['device_name']

                # Disable relays for release
                logger.info("Disable relays later")
                # config['options']['relaysEnabled'] = False

                response = await client.put(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}, data=json.dumps(config))
                logger.info(f"Local config update: {response.status_code==200}")

                if settings.REMOTE_URL != 'localhost':
                    response = await client.post(f"{settings.REMOTE_URL}/syncthing/connect", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}, data=json.dumps({'device_id': local_id}))
                    logger.info(f"Remote config update: {response.status_code==200}")
        except Exception as e:
            logger.error(f"Caught exception on syncthing setup: {e}")
            logger.info("Bye-bye")
            os._exit(0)

    async def terminate(self):
        try:
            async with httpx.AsyncClient() as client:
                result = await client.post(f"{settings.SYNCTHING_URL}/rest/system/shutdown", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})
                await self.process.wait()
        except Exception as e:
            logger.error(f"Caught exception on syncthing exit: {e}")

    async def get_events(self, client):
        try:
            events = (await client.get(f"{settings.SYNCTHING_URL}/rest/events?since={self.max_event_id}&timeout=120", headers={'Authorization': F'Bearer {settings.SYNCTHING_API_KEY}'})).json()
        except:
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
                logger.error("Can't find local device")
            return existing_device

    async def get_config(self):
        async with httpx.AsyncClient() as client:
            return (await client.get(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()

    async def append_device(self, device_id):
        async with httpx.AsyncClient() as client:
            config = (await client.get(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
            device = (await client.get(f"{settings.SYNCTHING_URL}/rest/config/defaults/device", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})).json()
            device['deviceID'] = device_id
            device['name'] = f'Client #{device_id}'

            config['devices'].append(device)
            config['folders'][0]['devices'].append({
                'deviceID': device_id,
                'introducedBy': '',
                'encryptionPassword': ''
            })

            response = await client.put(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}, data=json.dumps(config))
            logger.info(f"Local config update: {response.status_code==200}")

syncthing_manager = SyncthingManager("syncthing.exe", settings.WORKSPACE_DIR)
