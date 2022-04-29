#ifndef USD_REMOTE_ASSET_RESOLVER_CONTEXT_H
#define USD_REMOTE_ASSET_RESOLVER_CONTEXT_H

#include <pxr/pxr.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/ar/defineResolverContext.h>

#include <string>
#include <vector>
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

class WebUsdAssetResolverContext
{
public:
    AR_API
    WebUsdAssetResolverContext() = default;        

    AR_API 
    WebUsdAssetResolverContext(const std::string& value);

    AR_API
    std::string GetValue() const;

    AR_API 
    bool operator<(const WebUsdAssetResolverContext& rhs) const;
    
    AR_API 
    bool operator==(const WebUsdAssetResolverContext& rhs) const;
    
    AR_API 
    bool operator!=(const WebUsdAssetResolverContext& rhs) const;

private:
    std::string mUuid;
};

AR_API size_t hash_value(const WebUsdAssetResolverContext& context);

AR_DECLARE_RESOLVER_CONTEXT(WebUsdAssetResolverContext);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_REMOTE_ASSET_RESOLVER_CONTEXT_H
