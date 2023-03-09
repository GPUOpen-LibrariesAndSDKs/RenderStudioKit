#ifndef USD_REMOTE_ASSET_RESOLVER_H
#define USD_REMOTE_ASSET_RESOLVER_H

#include <filesystem>

#include <pxr/pxr.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/defaultResolver.h>

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
    static void SetRemoteServerAddress(const std::string& url);

    AR_API
    virtual std::string _GetExtension(const std::string& path) const override;

    AR_API
    virtual ArResolvedPath _Resolve(const std::string& path) const override;

protected:
    AR_API
    virtual std::shared_ptr<ArAsset> _OpenAsset(const ArResolvedPath& resolvedPath) const override;
    

    AR_API
    virtual ArTimestamp _GetModificationTimestamp(const std::string& path, const ArResolvedPath& resolvedPath) const override;

private:
    static std::filesystem::path GetDocumentsDirectory();

    static inline std::string sRemoteUrl;
    static inline RenderStudioFileFormatPtr sFileFormat;

    std::filesystem::path mRootPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
