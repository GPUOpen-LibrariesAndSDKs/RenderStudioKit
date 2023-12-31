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

#include "Resolver.h"

#pragma warning(push, 0)
#include <pxr/base/arch/env.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/usd/ar/defineResolver.h>
#include <pxr/usd/sdf/fileFormat.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#ifdef PLATFORM_WINDOWS
#include <shlobj.h>
#include <windows.h>
#pragma comment(lib, "shell32.lib")
#elif PLATFORM_UNIX
#include <pwd.h>
#include <unistd.h>

#include <sys/types.h>
#endif
#pragma warning(pop)

#include "../Kit.h"
#include "Asset.h"

#include <Logger/Logger.h>
#include <Networking/LocalStorageApi.h>
#include <Networking/MaterialLibraryApi.h>
#include <Networking/RestClient.h>
#include <Networking/Url.h>
#include <Utils/FileUtils.h>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(RenderStudioResolver, ArResolver)

std::unique_ptr<RenderStudio::Kit::LiveSessionInfo> RenderStudioResolver::sLiveModeInfo = nullptr;

bool
RenderStudioResolver::IsRenderStudioPath(const std::string& path)
{
    if (path.rfind("studio:/", 0) == 0)
    {
        return true;
    }

    if (path.rfind("gpuopen:/", 0) == 0)
    {
        return true;
    }

    if (path.rfind("storage:/", 0) == 0)
    {
        return true;
    }

    return false;
}

bool
RenderStudioResolver::IsUnresolvable(const std::string& path)
{
    if (path.find("://") != std::string::npos)
        return false;

    std::filesystem::path relative
        = std::filesystem::path { path }.lexically_relative(RenderStudioResolver::GetRootPath());
    return !relative.empty() && *relative.begin() != "..";
}

void
RenderStudioResolver::SetWorkspacePath(const std::string& path)
{
    sWorkspacePath = path;
}

std::string
RenderStudioResolver::GetWorkspacePath()
{
    return GetRootPath().string();
}

std::string
RenderStudioResolver::Unresolve(const std::string& path)
{
    std::string relative
        = std::filesystem::path { path }.lexically_relative(RenderStudioResolver::GetRootPath()).string();
    std::replace(relative.begin(), relative.end(), '\\', '/');
    return "studio://" + relative;
}

RenderStudioResolver::RenderStudioResolver()
{
    std::filesystem::path rootPath = RenderStudioResolver::GetRootPath();

    if (!std::filesystem::exists(rootPath))
    {
        std::filesystem::create_directory(rootPath);
    }

    if (sFileFormat == nullptr)
    {
        SdfFileFormatConstPtr format = SdfFileFormat::FindByExtension(".studio");
        RenderStudioFileFormatConstPtr casted = TfDynamic_cast<RenderStudioFileFormatConstPtr>(format);
        sFileFormat = TfConst_cast<RenderStudioFileFormatPtr>(casted);
    }

    if (sFileFormat == nullptr)
    {
        throw std::runtime_error("Can't access RenderStudioFileFormat");
    }

    LOG_INFO << "[RenderStudioResolver] Workspace folder: " << rootPath;
}

RenderStudioResolver::~RenderStudioResolver() { }

bool
RenderStudioResolver::ProcessLiveUpdates()
{
    return sFileFormat->ProcessLiveUpdates();
}

void
RenderStudioResolver::StartLiveMode(const RenderStudio::Kit::LiveSessionInfo& info)
{
    RenderStudio::Kit::LiveSessionInfo copy = info;

    if (copy.liveUrl.find("localhost") != std::string::npos)
    {
        boost::algorithm::replace_all(copy.liveUrl, "/workspace/live", ":52702");
        LOG_INFO << "Replaced live URL for localhost: " << info.liveUrl << " -> " << copy.liveUrl;
    }

    if (copy.liveUrl.find("127.0.0.1") != std::string::npos)
    {
        boost::algorithm::replace_all(copy.liveUrl, "/workspace/live", ":52702");
        LOG_INFO << "Replaced live URL for localhost: " << info.liveUrl << " -> " << copy.liveUrl;
    }

    // Connect here for now
    sLiveModeInfo = std::make_unique<RenderStudio::Kit::LiveSessionInfo>(copy);
    sFileFormat->Connect(copy.liveUrl + "/" + info.channelId + "/?user=" + info.userId);
}

void
RenderStudioResolver::StopLiveMode()
{
    sFileFormat->Disconnect();
}

ArResolvedPath
RenderStudioResolver::_Resolve(const std::string& path) const
{
    // If it's RenderStudio path or GpuOpen root material, do not resolve here
    // In case of resolving here, USD would use own SdfFileFormat instead of ours
    // Exception is .mtlx or .hdr that already stored in scene

    // If it's texture from GpuOpen asset, resolve to physical location here
    if (path.rfind("gpuopen:/", 0) == 0)
    {
        // If we have more than two tokens it means USD looking for already downloaded texture
        // For example: gpuopen://uuid/texture/path.png, so resolve to physical location
        auto tokens = TfStringTokenize(path, "/");
        if (tokens.size() > 2)
        {
            std::string uuid = tokens.at(1);
            std::filesystem::path location = RenderStudioResolver::GetAssetsCachePath() / "Materials";
            for (std::size_t i = 1; i < tokens.size(); i++)
            {
                location /= tokens.at(i);
            }
            return ArResolvedPath(location.string());
        }

        return ArResolvedPath(path);
    }

    // If it's texture from LocalStorage asset, resolve to physical location here
    if (path.rfind("storage:/", 0) == 0)
    {
        // If we have more than two tokens it means USD looking for already downloaded texture
        // For example: storage://uuid/light/path.png, so resolve to physical location
        auto tokens = TfStringTokenize(path, "/");
        if (tokens.size() > 2)
        {
            std::string uuid = tokens.at(1);
            std::filesystem::path location = RenderStudioResolver::GetAssetsCachePath() / "Storage";
            for (std::size_t i = 1; i < tokens.size(); i++)
            {
                location /= tokens.at(i);
            }
            return ArResolvedPath(location.string());
        }

        return ArResolvedPath(path);
    }

    // Special case for asset files that already been in scene
    if (std::filesystem::path(path).extension().string().find(".usd") == std::string::npos)
    {
        std::string copy = path;
        copy.replace(copy.find("studio:"), sizeof("studio:") - 1, RenderStudioResolver::GetRootPath().string());
        return ArResolvedPath(copy);
    }

    return ArResolvedPath(path);
}

AR_API std::string
RenderStudioResolver::ResolveImpl(const std::string& path)
{
    std::string copy = path;
    copy.erase(0, std::string("studio:/").size());
    if (copy.at(0) == '/')
    {
        copy.erase(0, 1);
    }
    std::filesystem::path resolved = RenderStudioResolver::GetRootPath() / copy;
    return resolved.string();
}

std::string
RenderStudioResolver::GetLocalStorageUrl()
{
    if (sLiveModeInfo != nullptr && !sLiveModeInfo->storageUrl.empty())
    {
        return sLiveModeInfo->storageUrl;
    }
    else if (ArchHasEnv("STORAGE_SERVER_URL"))
    {
        return ArchGetEnv("STORAGE_SERVER_URL");
    }
    else
    {
        return "https://renderstudio.matlib.gpuopen.com/workspace/storage";
    }
}

std::string
RenderStudioResolver::GetCurrentUserId()
{
    if (sLiveModeInfo != nullptr)
    {
        return sLiveModeInfo->userId;
    }
    else
    {
        return "";
    }
}

static std::string
_AnchorRelativePathForStudioProtocol(const std::string& anchorPath, const std::string& path)
{
    if (!RenderStudioResolver::IsRenderStudioPath(anchorPath) != 0
        && (TfIsRelativePath(anchorPath) || !TfIsRelativePath(path)))
    {
        return path;
    }

    // Ensure we are using forward slashes and not back slashes.
    std::string forwardPath = anchorPath;
    std::replace(forwardPath.begin(), forwardPath.end(), '\\', '/');

    std::string anchoredPath;

    // Render Studio paths would be relative to current asset, so take parent folder name and add path
    if (anchorPath.rfind("studio:/", 0) == 0)
    {
        anchoredPath = TfStringCatPaths(TfStringGetBeforeSuffix(forwardPath, '/'), path);
    }

    // GPUOpen paths would be global, so we need to persist current asset name (with uuid) in identifier
    if (anchorPath.rfind("gpuopen:/", 0) == 0)
    {
        // For now only gpuopen:// supports query parameters. Need to remove them before constructing full path
        const RenderStudio::Networking::Url url = RenderStudio::Networking::Url::Parse(anchorPath);

        std::string uuid = url.Path();
        uuid.erase(std::remove(uuid.begin(), uuid.end(), '/'), uuid.end());
        forwardPath = url.Protocol() + ":/" + uuid;

        anchoredPath = TfStringCatPaths(forwardPath, path);
    }

    // LocalStorage paths would be global, so we need to persist current asset name (with uuid) in identifier
    if (anchorPath.rfind("storage:/", 0) == 0)
    {
        anchoredPath = TfStringCatPaths(forwardPath, path);
    }

    return TfNormPath(anchoredPath);
}

AR_API std::string
RenderStudioResolver::_CreateIdentifier(const std::string& assetPath, const ArResolvedPath& anchorAssetPath) const
{
    if (assetPath.empty())
    {
        return assetPath;
    }

    if (!anchorAssetPath)
    {
        return TfNormPath(assetPath);
    }

    if (IsRenderStudioPath(assetPath))
    {
        return TfNormPath(assetPath);
    }

    const std::string anchoredAssetPath = _AnchorRelativePathForStudioProtocol(anchorAssetPath, assetPath);
    return TfNormPath(anchoredAssetPath);
}

std::shared_ptr<ArAsset>
RenderStudioResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const
{
    std::string _path = resolvedPath.GetPathString();

    if (resolvedPath.GetPathString().rfind("studio:/", 0) == 0)
    {
        RenderStudioNotice::LiveHistoryStatus notice(_path, "primitive");

        _path.erase(0, std::string("studio:/").size());
        std::filesystem::path resolved = RenderStudioResolver::GetRootPath() / _path;
        return ArDefaultResolver::_OpenAsset(ArResolvedPath { resolved.string() });
    }

    if (resolvedPath.GetPathString().rfind("gpuopen:/", 0) == 0)
    {
        std::optional<RenderStudioNotice::LiveHistoryStatus> notice;

        const RenderStudio::Networking::Url url = RenderStudio::Networking::Url::Parse(resolvedPath.GetPathString());

        std::string uuid = url.Path();
        uuid.erase(std::remove(uuid.begin(), uuid.end(), '/'), uuid.end());

        if (auto query = url.Query(); query.count("title") > 0)
        {
            notice.emplace(query["title"], "material");
        }
        else
        {
            notice.emplace(uuid, "material");
        }

        std::filesystem::path saveLocation = RenderStudioResolver::GetAssetsCachePath() / "Materials" / uuid;
        return GpuOpenAsset::Open(uuid, saveLocation);
    }

    if (resolvedPath.GetPathString().rfind("storage:/", 0) == 0)
    {
        RenderStudioNotice::LiveHistoryStatus notice(_path, "light");

        std::string name = resolvedPath.GetPathString();
        name.erase(0, std::string("storage:/").size());

        std::filesystem::path saveLocation = RenderStudioResolver::GetAssetsCachePath() / "Storage" / name;
        return LocalStorageAsset::Open(name, saveLocation);
    }

    return ArDefaultResolver::_OpenAsset(ArResolvedPath { resolvedPath });
}

std::string
RenderStudioResolver::_GetExtension(const std::string& assetPath) const
{
    (void)assetPath;
    return "studio";
}

std::filesystem::path
RenderStudioResolver::GetRootPath()
{
    return sWorkspacePath.empty() ? RenderStudio::Utils::GetDefaultWorkspacePath() : sWorkspacePath;
}

std::filesystem::path
RenderStudioResolver::GetAssetsCachePath()
{
    return RenderStudio::Utils::GetCachePath() / "assets";
}

PXR_NAMESPACE_CLOSE_SCOPE
