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

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from pydantic import BaseModel
import asyncio
import sys

from app.logger import logger
from app.settings import settings
from app.connection_manager import connection_manager
from app.syncthing_manager import syncthing_manager

app = FastAPI()

@app.on_event("startup")
async def startup_event():
    logger.info("Startup event which should check remote")

@app.websocket("/studio/watchdog")
async def watchdog(websocket: WebSocket):
    await connection_manager.connect(websocket)
    try:
        while True:
            await connection_manager.receive_text(websocket)
    except WebSocketDisconnect:
        await connection_manager.disconnect(websocket, should_close=False)

class ConnectionInfo(BaseModel):
    device_id: str

@app.get("/syncthing/info")
async def info():
    device = await syncthing_manager.get_device()
    config = await syncthing_manager.get_config()
    return {
        'device_id': device['deviceID'],
        'device_name': device['name'],
        'folder_id': config['folders'][0]['id'],
        'folder_name': config['folders'][0]['label']
    }


@app.post("/connect")
async def connect(info: ConnectionInfo):
    await syncthing_manager.append_device(info.device_id)
    return {'status': 'ok'}
