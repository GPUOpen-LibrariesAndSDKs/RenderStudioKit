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

#include <boost/json/src.hpp>
#pragma comment(lib, "shell32.lib")

#include "RestClient.h"

#include <Logger/Logger.h>
#include <Notice/Notice.h>
#include <Utils/FileUtils.h>

namespace RenderStudio::Networking
{

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

std::string
Syncthing::GetWorkspaceUrl()
{
    return sWorkspaceUrl;
}

void
Syncthing::Connect()
{
    // Create task
    if (sPingTask != nullptr)
    {
        sPingTask->Stop();
        LOG_DEBUG << "[RenderStudio Kit] Successfully killed old ping task";
    }

    sPingTask = std::make_shared<RenderStudio::Utils::BackgroundTask>([] { sClient->Send("ping"); }, 5);

    // Create client
    if (sClient != nullptr)
    {
        sClient->Disconnect().get();
        LOG_DEBUG << "[RenderStudio Kit] Successfully disconnected old client";
    }

    auto endpoint = RenderStudio::Networking::Url::Parse("ws://127.0.0.1:52700/studio/watchdog");

    // Check if watchdog already running
    sClient = Syncthing::CreateClient();
    bool connected = sClient->Connect(endpoint).get();

    if (connected)
    {
        // Watchdog already running
        pxr::RenderStudioNotice::WorkspaceConnectionChanged(true).Send();
        sPingTask->Start();
        LOG_INFO << "[RenderStudio Kit] Watchdog already launched, skipping";
    }
    else
    {
        // Launch new instance
        bool status = Syncthing::LaunchWatchdog();

        if (!status)
        {
            LOG_FATAL << "[RenderStudio Kit] Can't launch watchdog";
            return;
        }

        // To have more chances it's already launched
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Connect new client to that instance
        sClient->Disconnect().get();
        sClient = Syncthing::CreateClient();
        connected = sClient->Connect(endpoint).get();

        pxr::RenderStudioNotice::WorkspaceConnectionChanged(connected).Send();

        if (!connected)
        {
            LOG_FATAL << "[RenderStudio Kit] Can't connect to watchdog";
            return;
        }

        sPingTask->Start();
    }
}

void
Syncthing::Disconnect()
{
    sPingTask->Stop();
    bool status = sClient->Disconnect().get();
    (void)status; // Ignore disconnect status in notice
    pxr::RenderStudioNotice::WorkspaceConnectionChanged(false).Send();
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
        throw std::exception("Could not create child process");
    }
    else
    {
        LOG_INFO << "[RenderStudio Kit] Launched child process " << app << " " << arg;
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

bool
Syncthing::LaunchWatchdog()
{
    pxr::TfType type = pxr::PlugRegistry::FindTypeByName("RenderStudioResolver");
    pxr::PlugPluginPtr plug = pxr::PlugRegistry::GetInstance().GetPluginForType(type);
    std::filesystem::path watchdogExePath
        = std::filesystem::path(plug->GetPath()).parent_path() / "RenderStudioWatchdog.exe";
    std::filesystem::path workspacePath
        = sWorkspacePath.empty() ? RenderStudio::Utils::GetDefaultWorkspacePath() : sWorkspacePath;
    std::string workspaceUrl = sWorkspaceUrl;

    if (!std::filesystem::exists(workspacePath))
    {
        std::filesystem::create_directories(workspacePath);
    }

    LOG_INFO << "[RenderStudio Kit] Watchdog exe path: " << watchdogExePath;
    LOG_INFO << "[RenderStudio Kit] Workspace path: " << workspacePath;
    LOG_INFO << "[RenderStudio Kit] Workspace url: " << workspaceUrl;

    try
    {
        Syncthing::LaunchProcess(
            watchdogExePath.make_preferred().string(),
            "--workspace \"" + workspacePath.make_preferred().string()
                + "\" "
                  "--remote-url "
                + workspaceUrl
                + " "
                  "--ping-interval 10");

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_FATAL << "[RenderStudio Kit] " << e.what();
        return false;
    }
}

std::shared_ptr<WebsocketClient>
Syncthing::CreateClient()
{
    return WebsocketClient::Create(
        [](const std::string& message)
        {
            boost::json::object json = boost::json::parse(message).as_object();
            std::string event = boost::json::value_to<std::string>(json.at("event"));
            if (event == "Event::FileUpdated")
            {
                std::string path = boost::json::value_to<std::string>(json.at("path"));
                pxr::RenderStudioNotice::FileUpdated(path).Send();
            }
            else if (event == "Event::StateChanged")
            {
                std::string state = boost::json::value_to<std::string>(json.at("state"));
                pxr::RenderStudioNotice::WorkspaceState(state).Send();
            }
        });
}

} // namespace RenderStudio::Networking
