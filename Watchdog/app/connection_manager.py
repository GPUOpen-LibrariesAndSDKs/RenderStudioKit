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
from typing import List

from app.logger import logger
from app.logic import on_first_client_connected, on_last_client_disconnected
from app.settings import settings
from app.syncthing_manager import syncthing_manager

class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []
        self.last_ping_times = {}

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        if not self.active_connections:
            asyncio.create_task(self.poll_syncthing())
            on_first_client_connected()
        self.active_connections.append(websocket)
        self.last_ping_times[websocket] = asyncio.get_running_loop().time()
        logger.info(f"Connected client. Active count: {len(self.active_connections)}")

    async def disconnect(self, websocket: WebSocket):
        self.active_connections.remove(websocket)
        del self.last_ping_times[websocket]
        logger.info(f"Disconnected client. Active count: {len(self.active_connections)}")
        if not self.active_connections:
            on_last_client_disconnected()

    async def receive_ping(self, websocket: WebSocket):
        self.last_ping_times[websocket] = asyncio.get_running_loop().time()

    async def check_clients(self):
        while True:
            current_time = asyncio.get_running_loop().time()
            for ws, last_ping_time in list(self.last_ping_times.items()):
                if current_time - last_ping_time > settings.REQUIRED_PING_INTERVAL_SECONDS:
                    await ws.close()
            await asyncio.sleep(5)

    async def broadcast(self, message):
        for ws, last_ping_time in list(self.last_ping_times.items()):
            await ws.send_json(message)

    async def poll_syncthing(self):
        while True:
            logger.info("Polling")
            events = syncthing_manager.get_events()
            for event in events:
                if event['type'] == 'ItemFinished':
                    path = 'studio://' + event['data']['item'].replace('\\', '/')
                    logger.info(path)
                    await self.broadcast({
                        'event': 'Event::FileUpdated',
                        'path': path
                    })
                elif event['type'] == 'StateChanged':
                    state = event['data']['to']
                    logger.info(state)
                    await self.broadcast({
                        'event': 'Event::StateChanged',
                        'state': state
                    })

connection_manager = ConnectionManager()
