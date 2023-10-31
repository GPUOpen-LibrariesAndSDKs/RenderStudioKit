// Copyright 2023 Advanced Micro Devices, Inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

#include "WebsocketClient.h"

#include <Utils/BackgroundTask.h>

namespace RenderStudio::Networking
{

class Syncthing
{
public:
    static void Connect();
    static void Disconnect();

    static void SetWorkspacePath(const std::string& path);

    static void SetWorkspaceUrl(const std::string& url);
    static std::string GetWorkspaceUrl();

private:
    // Configuration
    static inline std::filesystem::path sWorkspacePath;
    static inline std::string sWorkspaceUrl;

    // Connection
    static inline std::shared_ptr<WebsocketClient> sClient;
    static inline std::shared_ptr<RenderStudio::Utils::BackgroundTask> sPingTask;

    // Process utils
    static bool LaunchWatchdog();
    static std::shared_ptr<WebsocketClient> CreateClient();
};

} // namespace RenderStudio::Networking
