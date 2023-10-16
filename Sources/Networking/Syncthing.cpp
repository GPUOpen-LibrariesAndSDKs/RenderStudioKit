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
#include <boost/json.hpp>
#pragma comment(lib, "shell32.lib")

namespace RenderStudio::Networking
{

void
Syncthing::SetWorkspacePath(const std::string& path)
{
    sWorkspacePath = path;
}

void
Syncthing::LaunchInstance()
{
    if (CheckIfProcessIsActive(mProcess))
    {
        return;
    }

    pxr::TfType type = pxr::PlugRegistry::FindTypeByName("RenderStudioResolver");
    pxr::PlugPluginPtr plug = pxr::PlugRegistry::GetInstance().GetPluginForType(type);
    std::filesystem::path syncthingExe = std::filesystem::path(plug->GetPath()).parent_path() / "syncthing.exe";
    std::filesystem::path syncthingConfig = Syncthing::GetRootPath().parent_path() / ".syncthing";
    std::filesystem::path workspace = Syncthing::GetRootPath();

    if (!std::filesystem::exists(workspace))
    {
        std::filesystem::create_directories(workspace);
    }

    LOG_INFO << "[RenderStudio Kit] Syncthing exe path: " << syncthingExe;
    LOG_INFO << "[RenderStudio Kit] Syncthing config path: " << syncthingConfig;
    LOG_INFO << "[RenderStudio Kit] Workspace path: " << workspace;

    mProcess = Syncthing::LaunchProcess(
        syncthingExe.string(),
        "--home " + syncthingConfig.string()
            + " "
              "--no-default-folder "
              "--skip-port-probing "
              "--gui-address http://localhost:45454 "
              "--gui-apikey render-studio-key "
              "--no-browser "
              "--no-restart "
              "--no-upgrade "
              "--log-max-size=0 "
              "--log-max-old-files=0 ");

    RestClient client({ { RestClient::Parameters::Authorization, "Bearer render-studio-key" } });

    boost::json::object config = boost::json::parse(client.Get("http://localhost:45454/rest/config")).as_object();
    boost::json::object remote
        = boost::json::parse(client.Get("https://renderstudio.luxoft.com/storage/api/syncthing/info")).as_object();
    std::string id = boost::json::value_to<std::string>(
        boost::json::parse(client.Get("http://localhost:45454/rest/system/status")).at("myID"));

    // Setup folder
    boost::json::object folder
        = boost::json::parse(client.Get("http://localhost:45454/rest/config/defaults/folder")).as_object();
    folder.at("id") = remote.at("folder_id");
    folder.at("label") = remote.at("folder_name");
    folder.at("path") = workspace.string();
    folder.at("devices").as_array().push_back({
        { "deviceID", remote.at("device_id") },
        { "introducedBy", "" },
        { "encryptionPassword", "" },
    });
    config.at("folders").as_array().push_back(folder);

    // Trust remote device
    boost::json::object device
        = boost::json::parse(client.Get("http://localhost:45454/rest/config/defaults/device")).as_object();
    device.at("deviceID") = remote.at("device_id");
    device.at("name") = remote.at("device_name");
    config.at("devices").as_array().push_back(device);

    client.Put("http://localhost:45454/rest/config", boost::json::serialize(config));

    // Update remote config
    client.Post(
        "https://renderstudio.luxoft.com/storage/api/syncthing/connect",
        boost::json::serialize(boost::json::object { { "device_id", id } }));
}

void
Syncthing::KillInstance()
{
    StopProcess(mProcess);
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

    std::wstring input = app_w + L" " + arg_w;
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

bool
Syncthing::CheckIfProcessIsActive(PROCESS_INFORMATION pi)
{
    // Check if handle is closed
    if (pi.hProcess == NULL)
    {
        printf("Process handle is closed or invalid (%d).\n", GetLastError());
        return FALSE;
    }

    // If handle open, check if process is active
    DWORD lpExitCode = 0;
    if (GetExitCodeProcess(pi.hProcess, &lpExitCode) == 0)
    {
        printf("Cannot return exit code (%d).\n", GetLastError());
        throw std::exception("Cannot return exit code");
    }
    else
    {
        if (lpExitCode == STILL_ACTIVE)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

bool
Syncthing::StopProcess(PROCESS_INFORMATION& pi)
{
    // Check if handle is invalid or has allready been closed
    if (pi.hProcess == NULL)
    {
        printf("Process handle invalid. Possibly allready been closed (%d).\n", GetLastError());
        return 0;
    }

    // Terminate Process
    if (!TerminateProcess(pi.hProcess, 1))
    {
        printf("ExitProcess failed (%d).\n", GetLastError());
        return 0;
    }

    // Wait until child process exits.
    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED)
    {
        printf("Wait for exit process failed(%d).\n", GetLastError());
        return 0;
    }

    // Close process and thread handles.
    if (!CloseHandle(pi.hProcess))
    {
        printf("Cannot close process handle(%d).\n", GetLastError());
        return 0;
    }
    else
    {
        pi.hProcess = NULL;
    }

    if (!CloseHandle(pi.hThread))
    {
        printf("Cannot close thread handle (%d).\n", GetLastError());
        return 0;
    }
    else
    {
        pi.hProcess = NULL;
    }
    return 1;
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
