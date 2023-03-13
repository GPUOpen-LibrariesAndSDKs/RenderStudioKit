#include "Context.h"

PXR_NAMESPACE_OPEN_SCOPE

RenderStudioResolverContext::RenderStudioResolverContext(const std::string& value)
    : mUuid(value)
{
}

std::string
RenderStudioResolverContext::GetValue() const
{
    return mUuid;
}

bool
RenderStudioResolverContext::operator<(const RenderStudioResolverContext& rhs) const
{
    return GetValue() < rhs.GetValue();
}

bool
RenderStudioResolverContext::operator==(const RenderStudioResolverContext& rhs) const
{
    return GetValue() == rhs.GetValue();
}

bool
RenderStudioResolverContext::operator!=(const RenderStudioResolverContext& rhs) const
{
    return !(*this == rhs);
}

std::size_t
hash_value(const RenderStudioResolverContext& context)
{
    return std::hash<std::string> {}(context.GetValue());
}

PXR_NAMESPACE_CLOSE_SCOPE