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
from typing import List

from app.logger import logger
from app.settings import settings
from app.syncthing_manager import syncthing_manager

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

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        first_connection = not self.active_connections
        self.active_connections[websocket] = asyncio.get_running_loop().time()
        logger.info(f"Connected client, count: {len(self.active_connections)}")
        if first_connection and not self.syncthing_was_launched:
            self.syncthing_was_launched = True
            logger.info("First client connected, starting everything")
            asyncio.create_task(connection_manager.on_first_client_connected())
            asyncio.create_task(connection_manager.auto_disconnect_clients_job())
            asyncio.create_task(connection_manager.auto_poll_syncthing_job())
        else:
            if self.last_device_connected_event:
                await self.broadcast(self.last_device_connected_event)
            if self.last_folder_summary_event:
                await self.broadcast(self.last_folder_summary_event)

    async def disconnect(self, websocket: WebSocket, should_close: bool):
        if should_close:
            await websocket.close()

        if websocket in self.active_connections:
            del self.active_connections[websocket]
            logger.info(f"Disconnected client, count: {len(self.active_connections)}")
            await asyncio.sleep(5)
            last_connection = not self.active_connections
            if last_connection:
                logger.info("Last client disconnected, killing everything")
                await asyncio.create_task(connection_manager.on_last_client_disconnected())

    async def send_personal_message(self, message: str, websocket: WebSocket):
        await websocket.send_text(message)

    async def broadcast(self, message):
        for connection in self.active_connections.keys():
            await connection.send_json(message)

    async def receive_text(self, websocket: WebSocket):
        await websocket.receive_text()
        self.active_connections[websocket] = asyncio.get_running_loop().time()

    async def on_first_client_connected(self):
        await syncthing_manager.start()
        logger.info("Syncthing started and configured")

    async def on_last_client_disconnected(self):
        await syncthing_manager.terminate()
        logger.info("Syncthing stopped")
        await self.notify_connection_state_abort()
        logger.info("Bye-bye")
        os._exit(0)

    async def auto_disconnect_clients_job(self):
        while True:
            current_time = asyncio.get_running_loop().time()
            remove_list = []
            for websocket, time in self.active_connections.items():
                if current_time - time > settings.REQUIRED_PING_INTERVAL_SECONDS:
                    remove_list.append(websocket)
            for websocket in remove_list:
                await self.disconnect(websocket, should_close=True)
            await asyncio.sleep(5)

    async def auto_poll_syncthing_job(self):
        try:
            async with httpx.AsyncClient() as client:
                while True:
                    events = await syncthing_manager.get_events(client)
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
                    await asyncio.sleep(1)
        except Exception as e:
            logger.error(e)
            await self.on_last_client_disconnected()

    async def notify_connection_state_abort(self):
        self.last_device_connected_event = {
            'event': 'Event::Connection',
            'connected': False
        }
        await self.broadcast(self.last_device_connected_event)

connection_manager = ConnectionManager()