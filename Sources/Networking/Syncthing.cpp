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

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/thisPlugin.h>

#include <Logger/Logger.h>
#include <Notice/Notice.h>
#include <Utils/FileUtils.h>
#include <Utils/ProcessUtils.h>
#include <boost/json/src.hpp>

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
        LOG_DEBUG << "Successfully killed old ping task";
    }

    sPingTask = std::make_shared<RenderStudio::Utils::BackgroundTask>([] { sClient->Send("ping"); }, 5);

    // Create client
    if (sClient != nullptr)
    {
        sClient->Disconnect().get();
        LOG_DEBUG << "Successfully disconnected old client";
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
        LOG_INFO << "Watchdog already launched, skipping";
    }
    else
    {
        // Launch new instance
        bool status = Syncthing::LaunchWatchdog();

        if (!status)
        {
            LOG_FATAL << "Can't launch watchdog";
            return;
        }

        // Connect new client to that instance
        std::size_t attempt = 0;
        do
        {
            sClient->Disconnect().get();
            sClient = Syncthing::CreateClient();
            connected = sClient->Connect(endpoint).get();

            if (!connected)
            {
                attempt += 1;
                LOG_WARNING << "No connect to watchdog yet, attempt [" << attempt << "/5]";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } while (!connected && attempt < 5);

        pxr::RenderStudioNotice::WorkspaceConnectionChanged(connected).Send();

        if (!connected)
        {
            LOG_FATAL << "Can't connect to watchdog";
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

bool
Syncthing::LaunchWatchdog()
{
    pxr::TfType type = pxr::PlugRegistry::FindTypeByName("RenderStudioResolver");
    pxr::PlugPluginPtr plug = pxr::PlugRegistry::GetInstance().GetPluginForType(type);

#ifdef PLATFORM_WINDOWS
    std::filesystem::path watchdogExePath
        = std::filesystem::path(plug->GetPath()).parent_path() / "RenderStudioWatchdog.exe";
#endif

#ifdef PLATFORM_UNIX
    std::filesystem::path watchdogExePath
        = std::filesystem::path(plug->GetPath()).parent_path() / "RenderStudioWatchdog";
#endif

    std::filesystem::path workspacePath
        = sWorkspacePath.empty() ? RenderStudio::Utils::GetDefaultWorkspacePath() : sWorkspacePath;
    std::string workspaceUrl = sWorkspaceUrl;

    if (!std::filesystem::exists(workspacePath))
    {
        std::filesystem::create_directories(workspacePath);
    }

    LOG_INFO << "Watchdog path: " << watchdogExePath;
    LOG_INFO << "Workspace path: " << workspacePath;
    LOG_INFO << "Workspace url: " << workspaceUrl;

    try
    {
        RenderStudio::Utils::LaunchProcess(
            watchdogExePath,
            { "--workspace",
              "\"" + workspacePath.make_preferred().string() + "\"",
              "--remote-url",
              workspaceUrl,
              "--ping-interval",
              "10" });
    }
    catch (const std::exception& e)
    {
        LOG_FATAL << "" << e.what();
        return false;
    }

    return true;
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
