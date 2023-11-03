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

import logging

LOGGING_CONFIG = {
    "version": 1,
    "handlers": {
        "default": {
            "class": "logging.StreamHandler",
            "formatter": "workspace",
            "stream": "ext://sys.stderr"
        },
        "syncthing": {
            "class": "logging.StreamHandler",
            "formatter": "syncthing",
            "stream": "ext://sys.stderr"
        }
    },
    "formatters": {
        "workspace": {
            "format": "\033[94m[workspace]\033[0m %(message)s",
            "datefmt": "%H:%M:%S",
        },
        "syncthing": {
            "format": "\033[95m[syncthing]\033[0m %(message)s",
            "datefmt": "%H:%M:%S",
        }
    },
    'loggers': {
        'httpx': {
            'handlers': ['default'],
            'level': 'WARN',
        },
        'httpcore': {
            'handlers': ['default'],
            'level': 'WARN',
        },
        'default': {
            'handlers': ['default'],
            'level': 'INFO',
        },
        'syncthing': {
            'handlers': ['syncthing'],
            'level': 'INFO',
        }
    }
}

logging.config.dictConfig(LOGGING_CONFIG)
logger = logging.getLogger('default')
syncthing_logger = logging.getLogger('syncthing')
