#include "Context.h"

PXR_NAMESPACE_OPEN_SCOPE

WebUsdAssetResolverContext::WebUsdAssetResolverContext(const std::string& value)
    : mUuid(value)
{

}

std::string 
WebUsdAssetResolverContext::GetValue() const
{
    return mUuid;
}

bool
WebUsdAssetResolverContext::operator<(const WebUsdAssetResolverContext& rhs) const
{
    return GetValue() < rhs.GetValue();
}

bool 
WebUsdAssetResolverContext::operator==(const WebUsdAssetResolverContext& rhs) const
{
    return GetValue() == rhs.GetValue();
}

bool 
WebUsdAssetResolverContext::operator!=(const WebUsdAssetResolverContext& rhs) const
{
    return !(*this == rhs);
}

std::size_t
hash_value(const WebUsdAssetResolverContext& context) {
    return std::hash<std::string>{}(context.GetValue());
}

PXR_NAMESPACE_CLOSE_SCOPE