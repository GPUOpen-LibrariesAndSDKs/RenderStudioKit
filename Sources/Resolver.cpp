#include "Resolver.h"

#include <iostream>
#include <regex>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/usd/ar/defineResolver.h>
#include <pxr/usd/sdf/fileFormat.h>

#include "Asset.h"
#include "Logger/Logger.h"

#include <base64.hpp>
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

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(RenderStudioResolver, ArResolver);

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
    if (!sFileFormat)
    {
        SdfFileFormatConstPtr format = SdfFileFormat::FindByExtension(".studio");
        RenderStudioFileFormatConstPtr casted = TfDynamic_cast<RenderStudioFileFormatConstPtr>(format);
        sFileFormat = TfConst_cast<RenderStudioFileFormatPtr>(casted);

        // Connect here for now
        sFileFormat->Connect(sRemoteUrl);
    }

    sFileFormat->ProcessLiveUpdates();
}

void
RenderStudioResolver::SetRemoteServerAddress(const std::string& url)
{
    sRemoteUrl = url;

    LOG_INFO << "Set remote server address to " << url;
}

ArResolvedPath
RenderStudioResolver::_Resolve(const std::string& path) const
{
    // Not resolving here since USD stop using our resolver when sees regular .usd files
    return ArResolvedPath(path);
}

static std::string
_AnchorRelativePathForStudioProtocol(const std::string& anchorPath, const std::string& path)
{
    if (anchorPath.rfind("studio:/", 0) != 0 && (TfIsRelativePath(anchorPath) || !TfIsRelativePath(path)))
    {
        return path;
    }

    // Ensure we are using forward slashes and not back slashes.
    std::string forwardPath = anchorPath;
    std::replace(forwardPath.begin(), forwardPath.end(), '\\', '/');

    // If anchorPath does not end with a '/', we assume it is specifying
    // a file, strip off the last component, and anchor the path to that
    // directory.
    const std::string anchoredPath = TfStringCatPaths(TfStringGetBeforeSuffix(forwardPath, '/'), path);
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

    const std::string anchoredAssetPath = _AnchorRelativePathForStudioProtocol(anchorAssetPath, assetPath);

    return TfNormPath(anchoredAssetPath);
}

std::shared_ptr<ArAsset>
RenderStudioResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const
{
    std::string path = resolvedPath.GetPathString();
    if (path.rfind("studio:/", 0) == 0)
    {
        path.erase(0, std::string("studio:/").size());
        std::filesystem::path resolved = mRootPath / path;
        return ArDefaultResolver::_OpenAsset(ArResolvedPath { resolved.string() });
    }
    else
    {
        ArDefaultResolver::_OpenAsset(ArResolvedPath { resolvedPath });
    }
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
