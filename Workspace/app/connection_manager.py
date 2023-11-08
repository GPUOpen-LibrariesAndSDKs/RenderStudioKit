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

from fastapi import WebSocket
import asyncio
import threading
import os
import httpx
import traceback
from typing import List

from app.logger import logger, syncthing_logger
from app.settings import settings
from app.syncthing_manager import syncthing_manager
from app.terminator import terminator

class ConnectionManager:
    def __init__(self):
        self.active_connections = {}
        self.syncthing_was_launched = False

        if settings.REMOTE_URL == 'localhost':
            self.last_folder_summary_event = {
                'event': 'Event::StateChanged',
                'state': 'idle'
            }
            self.last_device_connected_event = {
                'event': 'Event::Connection',
                'connected': True
            }
        else:
            self.last_folder_summary_event = None
            self.last_device_connected_event = None

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        first_connection = not self.active_connections
        self.active_connections[websocket] = asyncio.get_running_loop().time()
        logger.info(f"[Websocket] Connected client, count: {len(self.active_connections)}")
        if first_connection and not self.syncthing_was_launched:
            self.syncthing_was_launched = True
            logger.info("[Websocket] First client connected, starting everything")
            asyncio.create_task(syncthing_manager.start())
            asyncio.create_task(connection_manager.auto_disconnect_clients_job())
            asyncio.create_task(connection_manager.auto_poll_syncthing_job())

            if settings.REMOTE_URL != 'localhost':
                asyncio.create_task(connection_manager.auto_check_connected_device())
            else:
                asyncio.create_task(connection_manager.auto_clean_devices_job())
        else:
            if self.last_device_connected_event:
                await self.broadcast(self.last_device_connected_event)
            if self.last_folder_summary_event:
                await self.broadcast(self.last_folder_summary_event)

    async def disconnect(self, websocket: WebSocket):
        try:
            await websocket.close()
        except Exception:
            logger.info(f"[Websocket] Caught exception during websocket close: {traceback.format_exc()} (That should be fine)")

        if not websocket in self.active_connections:
            logger.info("[Websocket] For some reason disconnected websocket wasn't in active connections list. Skipping")

        del self.active_connections[websocket]
        logger.info(f"[Websocket] Disconnected client, count: {len(self.active_connections)}")
        await asyncio.sleep(1)

        last_connection = not self.active_connections
        if last_connection:
            logger.info("[Websocket] Last client disconnected, killing everything")
            await asyncio.create_task(terminator.terminate_all())

    async def send_personal_message(self, message: str, websocket: WebSocket):
        await websocket.send_text(message)

    async def broadcast(self, message):
        for connection in self.active_connections.keys():
            await connection.send_json(message)

    async def receive_text(self, websocket: WebSocket):
        await websocket.receive_text()
        self.active_connections[websocket] = asyncio.get_running_loop().time()

    async def auto_disconnect_clients_job(self):
        try:
            while True:
                current_time = asyncio.get_running_loop().time()
                remove_list = []
                for websocket, time in self.active_connections.items():
                    if current_time - time > settings.REQUIRED_PING_INTERVAL_SECONDS:
                        remove_list.append(websocket)
                for websocket in remove_list:
                    await self.disconnect(websocket)
                await asyncio.sleep(5)
        except Exception:
            logger.error(f'[Websocket] Fatal error: {traceback.format_exc()}')
            await asyncio.create_task(terminator.terminate_all())

    async def auto_check_connected_device(self):
        try:
            while True:
                if self.last_device_connected_event:
                    break

                if await syncthing_manager.is_connected():
                    self.last_device_connected_event = {
                        'event': 'Event::Connection',
                        'connected': True
                    }
                    await self.broadcast(self.last_device_connected_event)
                    logger.info('[Websocket] Sent connected message from rest call!')
                    break

                await asyncio.sleep(1)
        except Exception:
            logger.error(f'[Websocket] Error in auto check connected device: {traceback.format_exc()}')

    async def auto_clean_devices_job(self):
        async with httpx.AsyncClient() as client:
            while True:
                try:
                    await syncthing_manager.cleanup_devices(client)
                    await asyncio.sleep(120)
                except Exception:
                    logger.error(f'[Websocket] Fatal error in auto clean devices: {traceback.format_exc()}')
                    await asyncio.create_task(terminator.terminate_all())

    async def auto_poll_syncthing_job(self):
        while True:
            try:
                events = await syncthing_manager.get_events()
                for event in events:
                    if event['type'] == 'ItemFinished':
                        path = 'studio:/' + event['data']['item'].replace('\\', '/')
                        await self.broadcast({
                            'event': 'Event::FileUpdated',
                            'path': path
                        })
                    elif event['type'] == 'StateChanged':
                        # Here broadcast only errors
                        state = event['data']['to']
                        if state == 'error':
                            await self.broadcast({
                                'event': 'Event::StateChanged',
                                'state': state
                            })
                    elif event['type'] == 'DeviceConnected':
                        logger.info(event)
                        self.last_device_connected_event = {
                            'event': 'Event::Connection',
                            'connected': True
                        }
                        await self.broadcast(self.last_device_connected_event)
                    elif event['type'] == 'FolderSummary':
                        state = "error"
                        if event['data']['summary']['globalBytes'] == 0:
                            state = "syncing"
                        elif event['data']['summary']['globalBytes'] != event['data']['summary']['inSyncBytes']:
                            state = "syncing"
                        elif event['data']['summary']['globalBytes'] == event['data']['summary']['inSyncBytes']:
                            state = "idle"
                        self.last_folder_summary_event = {
                            'event': 'Event::StateChanged',
                            'state': state
                        }
                        await self.broadcast(self.last_folder_summary_event)
                    elif event['type'] == 'PendingDevicesChanged':
                        logger.info(event)
                        if 'added' in event['data']:
                            for device in event['data']['added']:
                                await syncthing_manager.append_device(device)
                    elif event['type'] == 'PendingFoldersChanged':
                        logger.info(event)
                        if 'added' in event['data']:
                            for folder in event['data']['added']:
                                await syncthing_manager.append_folder(folder)
                    elif event['type'] == 'FolderErrors':
                        logger.info(event)
                        await syncthing_manager.fix_folder_errors()
                    elif event['type'] == 'DeviceDisconnected':
                        logger.info(event)
                        # if settings.REMOTE_URL == 'localhost':
                        #    await syncthing_manager.remove_device(event['data']['id'])
                await asyncio.sleep(1)
            except Exception:
                logger.error(f'[Websocket] Fatal error in auto poll: {traceback.format_exc()}')
                await asyncio.create_task(terminator.terminate_all())

    async def terminate(self):
        try:
            self.last_device_connected_event = {
                'event': 'Event::Connection',
                'connected': False
            }
            await self.broadcast(self.last_device_connected_event)
        except Exception:
            logger.error(f"[Terminate Websocket] Send disconnect event: {traceback.format_exc()} (that should be fine)")

        try:
            for websocket in list(self.active_connections.keys()):
                try:
                    await websocket.close()
                except Exception:
                    logger.error(f"[Terminate] Close websocket: {traceback.format_exc()} (that should be fine)")
        except Exception:
            logger.error(f"[Terminate Websocket] Close all websockets: {traceback.format_exc()} (that should be fine)")

connection_manager = ConnectionManager()
