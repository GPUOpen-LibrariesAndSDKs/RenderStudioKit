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

import os

from app.syncthing_manager import syncthing_manager
from app.logger import logger

# Start syncthing
def on_first_client_connected():
    logger.info("First client connected. Starting syncthing")
    syncthing_manager.start()


# Stop syncthing
def on_last_client_disconnected():
    logger.info("Last client disconnected. Terminating syncthing")
    syncthing_manager.terminate()
    os._exit(0)
