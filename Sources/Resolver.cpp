#include "Resolver.h"
#include "Asset.h"
#include "Logger/Logger.h"

#include <iostream>
#include <regex>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <pxr/usd/ar/defineResolver.h>
#include <pxr/base/tf/diagnostic.h>
#include <base64.hpp>
#include <pxr/usd/sdf/fileFormat.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#elif PLATFORM_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
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

RenderStudioResolver::~RenderStudioResolver()
{
    LOG_INFO << "Destroyed";
}

void
RenderStudioResolver::ProcessLiveUpdates()
{
    if (!sFileFormat)
    {
        SdfFileFormatConstPtr format = SdfFileFormat::FindByExtension(".studio");
        RenderStudioFileFormatConstPtr casted = TfDynamic_cast<RenderStudioFileFormatConstPtr>(format);
        sFileFormat = TfConst_cast<RenderStudioFileFormatPtr>(casted);
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
    return ArResolvedPath(path);
}

std::shared_ptr<ArAsset>
RenderStudioResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const
{
    std::string path = resolvedPath.GetPathString();
    if (path.rfind("studio:/", 0) == 0)
    {
        path.erase(0, std::string("studio:/").size());
        std::filesystem::path resolved = mRootPath / path;
        LOG_INFO << "Resolved from studio:// : " << resolved;
        return ArDefaultResolver::_OpenAsset(ArResolvedPath{ resolved.string() });
    }
    else
    {
        LOG_INFO << "Resolved from filesystem: " << path;
        ArDefaultResolver::_OpenAsset(ArResolvedPath{ resolvedPath });
    }
}

std::string
RenderStudioResolver::_GetExtension(const std::string& assetPath) const
{
    LOG_INFO << "Returning extension .studio";
    return "studio";
}

ArTimestamp
RenderStudioResolver::_GetModificationTimestamp(
    const std::string& path,
    const ArResolvedPath& resolvedPath) const
{
    return ArTimestamp(0);
}

std::filesystem::path RenderStudioResolver::GetDocumentsDirectory()
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

    if ((homedir = getenv("HOME")) == NULL) {
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
