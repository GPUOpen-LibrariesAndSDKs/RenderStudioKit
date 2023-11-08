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
import traceback

class Terminator:
    async def terminate_all(self):
        from app.logger import logger
        logger.info('[Terminator] TERMINATE ALL CALLED')

        try:
            from app.syncthing_manager import syncthing_manager
            await syncthing_manager.terminate()
        except Exception as e:
            logger.error(f"[Terminator] Syncthing: {traceback.format_exc()} (that should be fine)")

        try:
            from app.connection_manager import connection_manager
            await connection_manager.terminate()
        except Exception as e:
            logger.error(f"[Terminator] Websocket: {traceback.format_exc()} (that should be fine)")

        logger.info('Bye-bye')
        os._exit(1)

terminator = Terminator()
