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

#include "Workspace.h"

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
Workspace::SetWorkspacePath(const std::string& path)
{
    sWorkspacePath = path;
}

void
Workspace::SetWorkspaceUrl(const std::string& url)
{
    sWorkspaceUrl = url;
}

std::string
Workspace::GetWorkspaceUrl()
{
    return sWorkspaceUrl;
}

void
Workspace::Connect(RenderStudio::Kit::Role role)
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

    auto endpoint = RenderStudio::Networking::Url::Parse("ws://127.0.0.1:52700/workspace/ws");

    // Guard we are server, but port is busy
    if (role == RenderStudio::Kit::Role::Server && WebsocketClient::IsPortInUse(std::uint16_t { 52700 }))
    {
        LOG_FATAL << "Port 52700 is used. Can't launch server";
        return;
    }

    // Guard we are client which tries to connect to localhost
    if (role == RenderStudio::Kit::Role::Client
        && (sWorkspaceUrl.find("127.0.0.1") != std::string::npos
            || sWorkspaceUrl.find("localhost") != std::string::npos)
        && !WebsocketClient::IsPortInUse(std::uint16_t { 52700 }))
    {
        LOG_FATAL << "Server not running. If client tries to connect to localhost, server must be launched";
        return;
    }

    // Check if workspace already running
    bool connected = false;

    if (WebsocketClient::IsPortInUse(std::uint16_t { 52700 }))
    {
        LOG_DEBUG << "Port 52700 is used, trying to connect to existing workspace";
        sClient = WebsocketClient::Create(sLogic);
        connected = sClient->Connect(endpoint).get();
    }

    if (connected)
    {
        // Workspace already running
        sPingTask->Start();
        LOG_INFO << "Workspace already launched, skipping";
    }
    else
    {
        // Launch new instance
        bool status = Workspace::LaunchWorkspace();

        if (!status)
        {
            LOG_FATAL << "Can't launch workspace";
            return;
        }

        if (sClient != nullptr)
        {
            sClient->Disconnect().get();
        }

        // Connect new client to that instance
        std::size_t attempt = 0;
        do
        {
            sClient = WebsocketClient::Create(sLogic);
            connected = sClient->Connect(endpoint).get();

            if (!connected)
            {
                attempt += 1;
                LOG_WARNING << "No connect to workspace yet, attempt [" << attempt << "/5]";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } while (!connected && attempt < 5);

        if (!connected)
        {
            LOG_FATAL << "Can't connect to workspace";
            return;
        }

        sPingTask->Start();
    }
}

void
Workspace::Disconnect()
{
    sPingTask->Stop();
    bool status = sClient->Disconnect().get();
    (void)status; // Ignore disconnect status in notice
    pxr::RenderStudioNotice::WorkspaceConnectionChanged(false).Send();
}

void
Workspace::WaitIdle()
{
    LOG_INFO << "Waiting workspace to become idle";
    std::unique_lock lock(sStateMutex);
    sStateConditionVariable.wait(lock, [] { return sLastState == "idle" && sConnected; });
    LOG_INFO << "Workspace became idle";
}

bool
Workspace::IsIdle()
{
    if (sWorkspaceUrl.empty())
    {
        return true;
    }

    std::unique_lock lock(sStateMutex);
    return sLastState == "idle";
}

bool
Workspace::IsConnected()
{
    if (sWorkspaceUrl.empty())
    {
        return false;
    }

    std::unique_lock lock(sStateMutex);
    return sConnected;
}

bool
Workspace::LaunchWorkspace()
{
    pxr::TfType type = pxr::PlugRegistry::FindTypeByName("RenderStudioResolver");
    pxr::PlugPluginPtr plug = pxr::PlugRegistry::GetInstance().GetPluginForType(type);

#ifdef PLATFORM_WINDOWS
    std::filesystem::path workspaceExePath
        = std::filesystem::path(plug->GetPath()).parent_path() / "RenderStudioWorkspace" / "RenderStudioWorkspace.exe";
#endif

#ifdef PLATFORM_UNIX
    std::filesystem::path workspaceExePath
        = std::filesystem::path(plug->GetPath()).parent_path() / "RenderStudioWorkspace" / "RenderStudioWorkspace.bin";
#endif

    std::filesystem::path workspacePath
        = sWorkspacePath.empty() ? RenderStudio::Utils::GetDefaultWorkspacePath() : sWorkspacePath;
    std::string workspaceUrl = sWorkspaceUrl;

    if (!std::filesystem::exists(workspacePath))
    {
        std::filesystem::create_directories(workspacePath);
    }

    LOG_INFO << "Workspace exe: " << workspaceExePath;
    LOG_INFO << "Workspace path: " << workspacePath;
    LOG_INFO << "Workspace url: " << workspaceUrl;

    try
    {
        RenderStudio::Utils::LaunchProcess(
            workspaceExePath,
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

void
Logic::OnConnected()
{
    LOG_INFO << "Connected RenderStudioKit with RenderStudioWatchdog";
}

void
Logic::OnDisconnected()
{
    LOG_INFO << "Disconnected RenderStudioKit from RenderStudioWatchdog";
}

void
Logic::OnMessage(const std::string& message)
{
    try
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
            LOG_INFO << "[Workspace] Event::StateChanged: " << state;
            pxr::RenderStudioNotice::WorkspaceState(state).Send();
            std::unique_lock lock(Workspace::sStateMutex);
            Workspace::sLastState = state;
            Workspace::sStateConditionVariable.notify_all();
        }
        else if (event == "Event::Connection")
        {
            bool connected = boost::json::value_to<bool>(json.at("connected"));
            LOG_INFO << "[Workspace] Event::Connection: " << connected;
            pxr::RenderStudioNotice::WorkspaceConnectionChanged(connected).Send();
            std::unique_lock lock(Workspace::sStateMutex);
            Workspace::sConnected = connected;
            Workspace::sStateConditionVariable.notify_all();
        }
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR << ex.what();
    }
}

} // namespace RenderStudio::Networking
