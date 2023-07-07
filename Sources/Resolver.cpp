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

#include "Asset.h"

#include <Logger/Logger.h>
#include <Networking/LocalStorageApi.h>
#include <Networking/MaterialLibraryApi.h>
#include <Networking/RestClient.h>
#include <Networking/Url.h>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(RenderStudioResolver, ArResolver)

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
RenderStudioResolver::IsUnresovableToRenderStudioPath(const std::string& path)
{
    std::filesystem::path relative
        = std::filesystem::path { path }.lexically_relative(RenderStudioResolver::GetRootPath());
    return !relative.empty() && *relative.begin() != "..";
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

    LOG_INFO << "RenderStudioResolver successfully created. Home folder: " << rootPath;
}

RenderStudioResolver::~RenderStudioResolver() { }

bool
RenderStudioResolver::ProcessLiveUpdates()
{
    return sFileFormat->ProcessLiveUpdates();
}

void
RenderStudioResolver::StartLiveMode(const LiveModeInfo& info)
{
    sLiveModeInfo = info;

    // Connect here for now
    sFileFormat->Connect(info.liveUrl + "/" + info.channelId + "/?user=" + info.userId);
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
            std::filesystem::path location = RenderStudioResolver::GetRootPath() / "Materials";
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
            std::filesystem::path location = RenderStudioResolver::GetRootPath() / "Storage";
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

std::string
RenderStudioResolver::GetLocalStorageUrl()
{
    if (!sLiveModeInfo.storageUrl.empty())
    {
        return sLiveModeInfo.storageUrl;
    }
    else
    {
        return ArchGetEnv("STORAGE_SERVER_URL");
    }
}

std::string
RenderStudioResolver::GetCurrentUserId()
{
    return sLiveModeInfo.userId;
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
        RenderStudioLoadingNotice notice(_path, "primitive");

        _path.erase(0, std::string("studio:/").size());
        std::filesystem::path resolved = RenderStudioResolver::GetRootPath() / _path;
        return ArDefaultResolver::_OpenAsset(ArResolvedPath { resolved.string() });
    }

    if (resolvedPath.GetPathString().rfind("gpuopen:/", 0) == 0)
    {
        std::optional<RenderStudioLoadingNotice> notice;

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

        std::filesystem::path saveLocation = RenderStudioResolver::GetRootPath() / "Materials" / uuid;
        return GpuOpenAsset::Open(uuid, saveLocation);
    }

    if (resolvedPath.GetPathString().rfind("storage:/", 0) == 0)
    {
        RenderStudioLoadingNotice notice(_path, "light");

        std::string name = resolvedPath.GetPathString();
        name.erase(0, std::string("storage:/").size());
        std::filesystem::path saveLocation = RenderStudioResolver::GetRootPath() / "Storage" / name;
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
RenderStudioResolver::GetDocumentsDirectory()
{
#ifdef PLATFORM_WINDOWS
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
#elif PLATFORM_UNIX
    const char* homedir = nullptr;

    if ((homedir = getenv("HOME")) == NULL)
    {
        homedir = getpwuid(getuid())->pw_dir;
    }

    if (homedir != nullptr)
    {
        return std::filesystem::path(homedir) / "Documents";
    }
    else
    {
        throw std::runtime_error("Can't find Documents folder on Linux");
    }
#endif
}

std::filesystem::path
RenderStudioResolver::GetRootPath()
{
    return RenderStudioResolver::GetDocumentsDirectory() / "AMD RenderStudio Home";
}

PXR_NAMESPACE_CLOSE_SCOPE
