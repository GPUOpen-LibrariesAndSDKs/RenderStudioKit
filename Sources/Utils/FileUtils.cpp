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

#include "FileUtils.h"

#include <shlobj.h>
#include <windows.h>

#include <Logger/Logger.h>

namespace RenderStudio::Utils
{

std::filesystem::path
GetProgramDataPath()
{
    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &path);
    if (SUCCEEDED(hr))
    {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        return result;
    }
    return {};
}

std::filesystem::path
GetDefaultWorkspacePath()
{
    std::filesystem::path path = GetProgramDataPath() / "AMD" / "AMD RenderStudio" / "Workspace";

    if (!std::filesystem::exists(path))
    {
        bool result = std::filesystem::create_directories(path);

        if (!result)
        {
            LOG_FATAL << "[RenderStudio Kit] Can't create workspace directory: " << path;
        }
    }

    return path;
}

} // namespace RenderStudio::Utils
