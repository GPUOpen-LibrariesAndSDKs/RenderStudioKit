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

#include "Syncthing.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/thisPlugin.h>

#include "RestClient.h"

#include <Logger/Logger.h>
#include <boost/json/src.hpp>
#pragma comment(lib, "shell32.lib")

namespace RenderStudio::Networking
{

void
Syncthing::BackgroundPolling()
{
    while (true)
    {
        std::lock_guard<std::mutex> lock(sBackgroundMutex);
        if (sClient != nullptr)
        {
            sClient->Send("ping");
        }
        std::this_thread::sleep_for(std::chrono::seconds(25));
    }
}

void
Syncthing::SetWorkspacePath(const std::string& path)
{
    sWorkspacePath = path;
}

void
Syncthing::SetWorkspaceUrl(const std::string& url)
{
    sWorkspaceUrl = url;
}

void
Syncthing::Connect()
{
    Disconnect();

    // Aquire here to prevent deadlock from Disconnect()
    std::lock_guard<std::mutex> lock(sBackgroundMutex);

    // Check if watchdog already running
    auto endpoint = RenderStudio::Networking::Url::Parse("ws://127.0.0.1:52700/studio/watchdog");
    auto client = WebsocketClient::Create([](const std::string& message) { (void)message; });
    client->Connect(endpoint);

    if (client->GetConnectionStatus().get())
    {
        LOG_WARNING << "Watchdog already launched, skipping";
        return;
    }

    // Launch watchdog
    pxr::TfType type = pxr::PlugRegistry::FindTypeByName("RenderStudioResolver");
    pxr::PlugPluginPtr plug = pxr::PlugRegistry::GetInstance().GetPluginForType(type);
    std::filesystem::path watchdogExePath
        = std::filesystem::path(plug->GetPath()).parent_path() / "RenderStudioWatchdog.exe";
    std::filesystem::path workspacePath = Syncthing::GetRootPath();
    std::string workspaceUrl = sWorkspaceUrl;

    if (!std::filesystem::exists(workspacePath))
    {
        std::filesystem::create_directories(workspacePath);
    }

    LOG_INFO << "[RenderStudio Kit] Watchdog exe path: " << watchdogExePath;
    LOG_INFO << "[RenderStudio Kit] Workspace path: " << workspacePath;
    LOG_INFO << "[RenderStudio Kit] Workspace url: " << workspaceUrl;

    Syncthing::LaunchProcess(
        watchdogExePath.make_preferred().string(),
        "--workspace \"" + workspacePath.make_preferred().string()
            + "\" "
              "--remote-url "
            + workspaceUrl
            + " "
              "--ping-interval 30");

    sClient = WebsocketClient::Create([](const std::string& message) { (void)message; });
    sClient->Connect(endpoint);
    sClient->GetConnectionStatus().get();
    sClient->Send("ping");

    if (sBackgroundThread == nullptr)
    {
        sBackgroundThread = std::make_shared<std::thread>(Syncthing::BackgroundPolling);

        // Detaching to run forever even if Disconnect() was called
        sBackgroundThread->detach();
    }
}

void
Syncthing::Disconnect()
{
    std::lock_guard<std::mutex> lock(sBackgroundMutex);

    if (sClient != nullptr)
    {
        sClient->Disconnect();
        sClient.reset();
    }
}

PROCESS_INFORMATION
Syncthing::LaunchProcess(std::string app, std::string arg)
{
    // Prepare handles.
    STARTUPINFOW si;
    PROCESS_INFORMATION pi; // The function returns this
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Prepare CreateProcess args
    std::wstring app_w;
    Syncthing::Widen(app, app_w);

    std::wstring arg_w;
    Syncthing::Widen(arg, arg_w);

    std::wstring input = L"\"" + app_w + L"\" " + arg_w;
    wchar_t* arg_concat = const_cast<wchar_t*>(input.c_str());
    const wchar_t* app_const = app_w.c_str();

    // Start the child process.
    if (!CreateProcessW(
            app_const,  // app path
            arg_concat, // Command line (needs to include app path as first argument. args seperated by whitepace)
            NULL,       // Process handle not inheritable
            NULL,       // Thread handle not inheritable
            FALSE,      // Set handle inheritance to FALSE
            0,          // No creation flags
            NULL,       // Use parent's environment block
            NULL,       // Use parent's starting directory
            &si,        // Pointer to STARTUPINFO structure
            &pi)        // Pointer to PROCESS_INFORMATION structure
    )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        throw std::exception("Could not create child process");
    }
    else
    {
        std::cout << "[          ] Successfully launched child process" << std::endl;
    }

    // Return process handle
    return pi;
}

// helper to widen a narrow UTF8 string in Win32
const wchar_t*
Syncthing::Widen(const std::string& narrow, std::wstring& wide)
{
    size_t length = narrow.size();

    if (length > 0)
    {
        wide.resize(length);
        length = MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), (int)length, (wchar_t*)wide.c_str(), (int)length);
        wide.resize(length);
    }
    else
    {
        wide.clear();
    }

    return wide.c_str();
}

std::filesystem::path
Syncthing::GetDocumentsDirectory()
{
    wchar_t folder[1024];
    HRESULT hr = SHGetFolderPathW(0, CSIDL_MYDOCUMENTS, 0, 0, folder);
    if (SUCCEEDED(hr))
    {
        char str[1024];
        std::size_t count;
        wcstombs_s(&count, str, sizeof(str), folder, sizeof(folder) / sizeof(wchar_t) - 1);
        return std::filesystem::path(str);
    }
    else
    {
        throw std::runtime_error("Can't find Documents folder on Windows");
    }
}

std::filesystem::path
Syncthing::GetRootPath()
{
    if (!sWorkspacePath.empty())
    {
        return sWorkspacePath;
    }

    return Syncthing::GetDocumentsDirectory() / "AMD RenderStudio Home";
}

} // namespace RenderStudio::Networking
