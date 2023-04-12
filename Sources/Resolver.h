#pragma once

#pragma warning(push, 0)
#include <filesystem>

#include <pxr/pxr.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/resolver.h>
#pragma warning(pop)

#include "Context.h"
#include "FileFormat.h"

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioResolver : public ArDefaultResolver
{
public:
    AR_API
    RenderStudioResolver();

    AR_API
    virtual ~RenderStudioResolver();

    AR_API
    static void ProcessLiveUpdates();

    AR_API
    static void StartLiveMode();

    AR_API
    static void StopLiveMode();

    AR_API
    static void SetRemoteServerAddress(const std::string& liveUrl, const std::string& storageUrl);

    AR_API
    static void SetCurrentUserId(const std::string& name);

    AR_API
    virtual std::string _GetExtension(const std::string& path) const override;

    AR_API
    virtual ArResolvedPath _Resolve(const std::string& path) const override;

    AR_API
    static std::string GetLocalStorageUrl();

    AR_API
    static std::string GetCurrentUserId();

protected:
    AR_API
    std::string _CreateIdentifier(const std::string& assetPath, const ArResolvedPath& anchorAssetPath) const override;

    AR_API
    virtual std::shared_ptr<ArAsset> _OpenAsset(const ArResolvedPath& resolvedPath) const override;

    AR_API
    virtual ArTimestamp
    _GetModificationTimestamp(const std::string& path, const ArResolvedPath& resolvedPath) const override;

private:
    static std::filesystem::path GetDocumentsDirectory();

    static inline std::string sLiveUrl;
    static inline std::string sStorageUrl;
    static inline std::string sUserId;
    static inline RenderStudioFileFormatPtr sFileFormat;

    std::filesystem::path mRootPath;
};

PXR_NAMESPACE_CLOSE_SCOPE
