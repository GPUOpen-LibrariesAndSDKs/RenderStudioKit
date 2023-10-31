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

#include "ProcessUtils.h"

#ifdef PLATFORM_WINDOWS
#include <shlobj.h>
#include <tchar.h>
#include <windows.h>
#pragma comment(lib, "shell32.lib")
#endif

#ifdef PLATFORM_UNIX
#include <spawn.h>
#include <unistd.h>
#include <wordexp.h>
#endif

#include <stdio.h>

#include <boost/algorithm/string/join.hpp>

namespace RenderStudio::Utils
{

#ifdef PLATFORM_UNIX
void
LaunchProcess(std::filesystem::path apppication, const std::vector<std::string>& args)
{
    std::string app = apppication.make_preferred().string();
    std::string arg = boost::algorithm::join(args, " ");

    std::vector<char*> argv;

    wordexp_t p;
    if (wordexp((app + " " + arg).c_str(), &p, 0))
    {
        throw std::runtime_error("Could not create child process");
    }

    for (std::size_t i = 0; i < p.we_wordc; i++)
    {
        argv.push_back(p.we_wordv[i]);
    }

    argv.push_back(nullptr);

    pid_t pid;
    int status = posix_spawn(&pid, app.c_str(), nullptr, nullptr, argv.data(), environ);

    if (status != 0)
    {
        throw std::runtime_error("Could not create child process");
    }
}
#endif

#ifdef PLATFORM_WINDOWS
void
LaunchProcess(std::filesystem::path application, const std::vector<std::string>& args)
{
    std::string app = application.make_preferred().string();
    std::string arg = boost::algorithm::join(args, " ");

    // Prepare handles.
    STARTUPINFOW si;
    PROCESS_INFORMATION pi; // The function returns this
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Prepare CreateProcess args
    std::wstring app_w;
    Widen(app, app_w);

    std::wstring arg_w;
    Widen(arg, arg_w);

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
}

// helper to widen a narrow UTF8 string in Win32
const wchar_t*
Widen(const std::string& narrow, std::wstring& wide)
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
#endif

} // namespace RenderStudio::Utils
