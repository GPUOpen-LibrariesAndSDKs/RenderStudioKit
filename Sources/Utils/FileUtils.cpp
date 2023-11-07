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

#ifdef PLATFORM_WINDOWS
#include <shlobj.h>
#include <windows.h>
#endif

#pragma warning(push, 0)
#include <zip.h>
#pragma warning(pop)

#include <Logger/Logger.h>
#include <Utils/Uuid.h>

namespace RenderStudio::Utils
{

std::filesystem::path
GetProgramDataPath()
{
#ifdef PLATFORM_WINDOWS
    /* PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &path);
    if (SUCCEEDED(hr))
    {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        return result;
    }
    return {};
    */
    return "C:/";
#endif

#ifdef PLATFORM_UNIX
    return std::filesystem::path("/var/lib/");
#endif
}

std::filesystem::path
GetDefaultWorkspacePath()
{
    std::filesystem::path workspacePath = GetRenderStudioPath() / "Workspace";
    std::filesystem::path syncthingPath = GetRenderStudioPath() / "Syncthing";
    std::filesystem::path markerPath = GetRenderStudioPath() / "Workspace" / ".stfolder";

    // When marker is missing, remove all syncthing database to resync
    if (!std::filesystem::exists(markerPath))
    {
        std::filesystem::remove_all(syncthingPath);
        std::filesystem::create_directories(markerPath);
    }

    return workspacePath;
}

std::filesystem::path
GetRenderStudioPath()
{
    std::filesystem::path path = GetProgramDataPath() / "AMD" / "AMD RenderStudio";

    if (!std::filesystem::exists(path))
    {
        std::filesystem::create_directories(path);
    }

    return path;
}

TempDirectory::TempDirectory()
{
    std::filesystem::path path;

    do
    {
        path = GetRenderStudioPath() / "Temp" / RenderStudio::Utils::GenerateUUID();
    } while (std::filesystem::exists(path));

    std::filesystem::create_directories(path);
    mPath = path;
    LOG_INFO << "Create temp dir: " << mPath;
}

TempDirectory::~TempDirectory()
{
    if (std::filesystem::exists(mPath))
    {
        std::filesystem::remove_all(mPath);
    }
}

std::filesystem::path
TempDirectory::Path() const
{
    return mPath;
}

void
Extract(const std::filesystem::path& archive, const std::filesystem::path& destination)
{
    if (!std::filesystem::exists(destination))
    {
        std::filesystem::create_directories(destination);
    }

    int arg = 0;
    auto status = zip_extract(
        archive.string().c_str(),
        destination.string().c_str(),
        [](auto a, auto b)
        {
            (void)a, void(b);
            return 0;
        },
        &arg);

    if (status != 0)
    {
        LOG_ERROR << "Cant extract " << archive << " to " << destination << ", error: " << status;
    }
}

void
Move(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    if (!std::filesystem::exists(destination))
    {
        std::filesystem::create_directories(destination);
    }

    for (std::filesystem::path p : std::filesystem::directory_iterator(source))
    {
        std::filesystem::rename(p, destination / p.filename());
    }
}

std::filesystem::path
GetCachePath()
{
    std::filesystem::path path = GetRenderStudioPath() / "Cache";

    if (!std::filesystem::exists(path))
    {
        std::filesystem::create_directories(path);
    }

    return path;
}

} // namespace RenderStudio::Utils
