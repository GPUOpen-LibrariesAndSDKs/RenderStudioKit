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
import requests
import traceback
import json

from app.logger import logger
from app.settings import settings

class SyncthingManager:
    def __init__(self, exe_path: str, workspace_path: str):
        self.process = None
        self.exe_path = exe_path
        self.workspace_path = Path(workspace_path)
        self.max_event_id = 0

    def start(self):
        self.process = subprocess.Popen([
            self.exe_path,
            f"--home={self.workspace_path.parent / 'Syncthing'}",
            "--no-default-folder",
            "--skip-port-probing",
            f"--gui-address={settings.SYNCTHING_URL}",
            f"--gui-apikey={settings.SYNCTHING_API_KEY}",
            "--no-browser",
            "--no-restart",
            "--no-upgrade",
            "--log-max-size=0",
            "--log-max-old-files=0"
        ])

        try:
            config = requests.get(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}).json()
            remote = requests.get(f"{settings.REMOTE_URL}/storage/api/syncthing/info", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}).json()
            local_id = requests.get(f"{settings.SYNCTHING_URL}/rest/system/status", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}).json()['myID']

            # Setup folder
            existing_folder = next((folder for folder in config['folders'] if folder['id'] == remote['folder_id']), None)
            if not existing_folder:
                folder = requests.get(f"{settings.SYNCTHING_URL}/rest/config/defaults/folder", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}).json()
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
                device = requests.get(f"{settings.SYNCTHING_URL}/rest/config/defaults/device", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}).json()
                device['deviceID'] = remote['device_id']
                device['name'] = remote['device_name']
                device['addresses'].append(settings.REMOTE_URL.replace('https:/', 'tcp:/') + '/syncthing:443')
                config['devices'].append(device)

            # Disable relays for release
            logger.info("Disable relays later")
            # config['options']['relaysEnabled'] = False

            response = requests.put(f"{settings.SYNCTHING_URL}/rest/config", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}, data=json.dumps(config))
            logger.info(f"Status for local config update: [{response.status_code}] {response.text}")

            response = requests.post(f"{settings.REMOTE_URL}/storage/api/syncthing/connect", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'}, data=json.dumps({'device_id': local_id}))
            logger.info(f"Status for remote config update: [{response.status_code}] {response.text}")
        except Exception as e:
            logger.error(traceback.format_exc())

    def terminate(self):
        result = requests.post(f"{settings.SYNCTHING_URL}/rest/system/shutdown", headers={'Authorization': f'Bearer {settings.SYNCTHING_API_KEY}'})
        logger.info(f"Terminate syncthing response: {result}")
        self.process.communicate()
        self.process = None

    def get_events(self):
        events = requests.get(f"{settings.SYNCTHING_URL}/rest/events?since={self.max_event_id}", headers={'Authorization': F'Bearer {settings.SYNCTHING_API_KEY}'}).json()
        first_poll = self.max_event_id == 0
        for event in events:
            self.max_event_id = max(self.max_event_id, event['id'])
        return [] if first_poll else events

syncthing_manager = SyncthingManager("syncthing.exe", settings.WORKSPACE_DIR)
