#include "Notice.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_TYPE(RenderStudioNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));

RenderStudioNotice::RenderStudioNotice(const SdfPath& path, bool deleted, bool appended)
    : mChangedPrim(path)
    , mWasDeleted(deleted)
    , mSpecWasCreated(appended)
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

bool
RenderStudioNotice::SpecWasCreated() const
{
    return mSpecWasCreated;
}

PXR_NAMESPACE_CLOSE_SCOPE
