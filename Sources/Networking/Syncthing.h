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

#include <windows.h>

#include "WebsocketClient.h"

namespace RenderStudio::Networking
{

class Syncthing
{
public:
    static void SetWorkspacePath(const std::string& path);
    static void SetWorkspaceUrl(const std::string& url);
    static void Connect();
    static void Disconnect();
    static std::string GetWorkspaceUrl();

private:
    static PROCESS_INFORMATION LaunchProcess(std::string app, std::string arg);
    static const wchar_t* Widen(const std::string& narrow, std::wstring& wide);
    static std::filesystem::path GetDocumentsDirectory();
    static std::filesystem::path GetRootPath();

    static void BackgroundPolling();
    static inline std::mutex sBackgroundMutex;
    static inline std::shared_ptr<std::thread> sBackgroundThread;

    static inline std::filesystem::path sWorkspacePath;
    static inline std::string sWorkspaceUrl;
    static inline std::shared_ptr<WebsocketClient> sClient;
};

} // namespace RenderStudio::Networking
