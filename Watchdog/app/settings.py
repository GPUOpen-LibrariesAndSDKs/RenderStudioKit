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

from threading import Lock

class Settings:
    REQUIRED_PING_INTERVAL_SECONDS: int = None
    WEBSOCKET_PORT: int = None
    SYNCTHING_URL: str = None
    REMOTE_URL: str = None
    SYNCTHING_API_KEY: str = None
    WORKSPACE_DIR: str = None

    def get_declared_fields(self):
        class_annotations = self.__annotations__
        return {field: getattr(self, field) for field in class_annotations}

settings = Settings()
