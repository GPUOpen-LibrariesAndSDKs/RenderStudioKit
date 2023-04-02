#include "Notice.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_TYPE(RenderStudioNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));

RenderStudioNotice::RenderStudioNotice(const SdfPath& path)
    : mChangedPrim(path)
{
}

SdfPath
RenderStudioNotice::GetChangedPrim() const
{
    return mChangedPrim;
}

PXR_NAMESPACE_CLOSE_SCOPE
