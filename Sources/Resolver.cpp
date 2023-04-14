#include "Resolver.h"

#pragma warning(push, 0)
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

#include "Asset.h"

#include <Logger/Logger.h>
#include <Networking/LocalStorageApi.h>
#include <Networking/MaterialLibraryApi.h>
#include <Networking/RestClient.h>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(RenderStudioResolver, ArResolver);

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

RenderStudioResolver::RenderStudioResolver()
{
    mRootPath = RenderStudioResolver::GetDocumentsDirectory() / "AMD RenderStudio Home";

    if (!std::filesystem::exists(mRootPath))
    {
        std::filesystem::create_directory(mRootPath);
    }

    LOG_INFO << "RenderStudioResolver successfully created. Home folder: " << mRootPath;
}

RenderStudioResolver::~RenderStudioResolver() { LOG_INFO << "Destroyed"; }

void
RenderStudioResolver::ProcessLiveUpdates()
{
    sFileFormat->ProcessLiveUpdates();
}

void
RenderStudioResolver::StartLiveMode()
{
    if (sLiveUrl.empty())
    {
        throw std::runtime_error("Remote URL wasn't set");
    }

    if (!sFileFormat)
    {
        SdfFileFormatConstPtr format = SdfFileFormat::FindByExtension(".studio");
        RenderStudioFileFormatConstPtr casted = TfDynamic_cast<RenderStudioFileFormatConstPtr>(format);
        sFileFormat = TfConst_cast<RenderStudioFileFormatPtr>(casted);
    }

    // Connect here for now
    sFileFormat->Connect(sLiveUrl);
}

void
RenderStudioResolver::StopLiveMode()
{
    sFileFormat->Disconnect();
}

void
RenderStudioResolver::SetRemoteServerAddress(const std::string& liveUrl, const std::string& storageUrl)
{
    sLiveUrl = liveUrl;
    sStorageUrl = storageUrl;

    LOG_INFO << "Set remote server address to " << liveUrl << ", " << storageUrl;
}

void
RenderStudioResolver::SetCurrentUserId(const std::string& id)
{
    sUserId = id;
}

ArResolvedPath
RenderStudioResolver::_Resolve(const std::string& path) const
{
    // If it's texture from GpuOpen asset, resolve to physical location here
    if (path.rfind("gpuopen:/", 0) == 0)
    {
        // If we have more than two tokens it means USD looking for already downloaded texture
        // For example: gpuopen://uuid/texture/path.png, so resolve to physical location
        auto tokens = TfStringTokenize(path, "/");
        if (tokens.size() > 2)
        {
            std::string uuid = tokens.at(1);
            std::filesystem::path location = mRootPath / "Materials";
            for (auto i = 1; i < tokens.size(); i++)
            {
                location /= tokens.at(i);
            }
            return ArResolvedPath(location.string());
        }
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
            std::filesystem::path location = mRootPath / "Storage";
            for (auto i = 1; i < tokens.size(); i++)
            {
                location /= tokens.at(i);
            }
            return ArResolvedPath(location.string());
        }
    }

    // Special case for asset files that already been in scene
    if (std::filesystem::path(path).extension().string().find(".usd") == std::string::npos)
    {
        std::string copy = path;
        copy.replace(copy.find("studio:"), sizeof("studio:") - 1, mRootPath.string());
        return ArResolvedPath(copy);
    }

    // If it's RenderStudio path or GpuOpen root material, do not resolve here
    // In case of resolving here, USD would use own SdfFileFormat instead of ours
    return ArResolvedPath(path);
}

std::string
RenderStudioResolver::GetLocalStorageUrl()
{
    return sStorageUrl;
}

std::string
RenderStudioResolver::GetCurrentUserId()
{
    return sUserId;
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
        _path.erase(0, std::string("studio:/").size());
        std::filesystem::path resolved = mRootPath / _path;
        return ArDefaultResolver::_OpenAsset(ArResolvedPath { resolved.string() });
    }

    if (resolvedPath.GetPathString().rfind("gpuopen:/", 0) == 0)
    {
        std::string uuid = resolvedPath.GetPathString();
        uuid.erase(0, std::string("gpuopen:/").size());
        std::filesystem::path saveLocation = mRootPath / "Materials" / uuid;
        return GpuOpenAsset::Open(uuid, saveLocation);
    }

    if (resolvedPath.GetPathString().rfind("storage:/", 0) == 0)
    {
        std::string name = resolvedPath.GetPathString();
        name.erase(0, std::string("storage:/").size());
        auto package = RenderStudio::Networking::LocalStorageAPI::GetLightPackage(
            name, RenderStudioResolver::GetLocalStorageUrl());
        std::filesystem::path saveLocation = mRootPath / "Storage" / package.name;
        return LocalStorageAsset::Open(package.id, saveLocation);
    }

    return ArDefaultResolver::_OpenAsset(ArResolvedPath { resolvedPath });
}

std::string
RenderStudioResolver::_GetExtension(const std::string& assetPath) const
{
    return "studio";
}

ArTimestamp
RenderStudioResolver::_GetModificationTimestamp(const std::string& path, const ArResolvedPath& resolvedPath) const
{
    return ArTimestamp(0);
}

std::filesystem::path
RenderStudioResolver::GetDocumentsDirectory()
{
#ifdef PLATFORM_WINDOWS
    wchar_t folder[1024];
    HRESULT hr = SHGetFolderPathW(0, CSIDL_MYDOCUMENTS, 0, 0, folder);
    if (SUCCEEDED(hr))
    {
        char str[1024];
        wcstombs(str, folder, 1023);
        return std::filesystem::path(str);
    }
    else
    {
        throw std::runtime_error("Can't find Documents folder on Windows");
    }
#elif PLATFORM_UNIX
    const char* homedir = nullptr;

    if ((homedir = getenv("HOME")) == NULL)
    {
        homedir = getpwuid(getuid())->pw_dir;
    }

    if (homedir != nullptr)
    {
        return std::filesystem::path(homedir) / "Documents"
    }
    else
    {
        throw std::runtime_error("Can't find Documents folder on Linux");
    }
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
