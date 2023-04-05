#include "Notice.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_TYPE(RenderStudioNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));

RenderStudioNotice::RenderStudioNotice(const SdfPath& path, bool deleted)
    : mChangedPrim(path)
    , mWasDeleted(deleted)
{
}

SdfPath
RenderStudioNotice::GetChangedPrim() const
{
    return mChangedPrim;
}

bool
RenderStudioNotice::WasDeleted() const
{
    return mWasDeleted;
}

PXR_NAMESPACE_CLOSE_SCOPE
