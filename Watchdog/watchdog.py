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

import argparse
import uvicorn

from app.logger import logger
from app.settings import settings

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="AMD RenderStudio watchdog service", allow_abbrev=False)
    parser.add_argument('-p', '--port', help="Port for watchdog", type=str, required=False, default=52700)
    parser.add_argument('-i', '--ping-interval', help="Required ping interval", type=int, required=False, default=10)
    parser.add_argument('-s', '--syncthing-url', help="Syncthing API url", type=str, required=False, default="http://localhost:52701")
    parser.add_argument('-r', '--remote-url', help="Remote Syncthing API url", type=str, required=True)
    parser.add_argument('-k', '--syncthing-api-key', help="Local Syncthing API key", type=str, required=False, default="render-studio-key")
    parser.add_argument('-w', '--workspace', help="Syncthing API key", type=str, required=True)
    args = parser.parse_args()

    settings.REQUIRED_PING_INTERVAL_SECONDS = args.ping_interval
    settings.WEBSOCKET_PORT = args.port
    settings.SYNCTHING_URL = args.syncthing_url
    settings.SYNCTHING_API_KEY = args.syncthing_api_key
    settings.WORKSPACE_DIR = args.workspace
    settings.REMOTE_URL = args.remote_url

    for key, value in settings.get_declared_fields().items():
        logger.info(f"{key}: {value}")

    import app.main
    uvicorn.run("app.main:app", port=settings.WEBSOCKET_PORT, log_level="warning")
