#pragma once

#pragma warning(push, 0)
#include <pxr/base/tf/instantiateType.h>
#include <pxr/base/tf/notice.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/sdf/path.h>
#pragma warning(pop)

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioNotice : public TfNotice
{
public:
    AR_API
    RenderStudioNotice(const SdfPath& path);

    AR_API
    SdfPath GetChangedPrim() const;

private:
    SdfPath mChangedPrim;
};

PXR_NAMESPACE_CLOSE_SCOPE
