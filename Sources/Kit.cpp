#include "Kit.h"
#include <Resolver/Resolver.h>

namespace RenderStudio::Kit
{

void
LiveSessionConnect(const LiveSessionInfo& info)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::StartLiveMode(info);
}

bool
LiveSessionUpdate()
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::ProcessLiveUpdates();
}

void
LiveSessionDisconnect()
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::StopLiveMode();
}

bool
IsUnresovableToRenderStudioPath(const std::string& path)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::IsUnresovableToRenderStudioPath(path);
}

std::string
UnresolveToRenderStudioPath(const std::string& path)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::Unresolve(path);
}

bool
IsRenderStudioPath(const std::string& path)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::IsRenderStudioPath(path);
}

} // namespace RenderStudio::Kit
