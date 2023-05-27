#include "Notice.h"

#pragma warning(push, 0)
#include <random>
#include <sstream>
#pragma warning(pop)

#include <Logger/Logger.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_TYPE(RenderStudioNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));
TF_INSTANTIATE_TYPE(RenderStudioLoadingNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));

namespace uuid
{
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

std::string
generate_uuid_v4()
{
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++)
    {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++)
    {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++)
    {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++)
    {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++)
    {
        ss << dis(gen);
    };
    return ss.str();
}
} // namespace uuid

RenderStudioNotice::RenderStudioNotice(const SdfPath& path, bool deleted, bool appended)
    : mChangedPrim(path)
    , mWasDeleted(deleted)
    , mSpecWasCreated(appended)
{
    if (!mChangedPrim.IsPrimPath())
    {
        mIsValid = false;
    }
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

bool
RenderStudioNotice::IsValid() const
{
    return mIsValid;
}

RenderStudioLoadingNotice::RenderStudioLoadingNotice(const std::string& name, const std::string& category)
    : mName(name)
    , mCategory(category)
    , mUuid(uuid::generate_uuid_v4())
{
    mAction = RenderStudioLoadingNotice::Action::Started;
    Send();
}

RenderStudioLoadingNotice::~RenderStudioLoadingNotice()
{
    mAction = RenderStudioLoadingNotice::Action::Finished;
    Send();
}

std::string
RenderStudioLoadingNotice::GetName() const
{
    return mName;
}

std::string
RenderStudioLoadingNotice::GetCategory() const
{
    return mCategory;
}

std::string
RenderStudioLoadingNotice::GetUuid() const
{
    return mUuid;
}

RenderStudioLoadingNotice::Action
RenderStudioLoadingNotice::GetAction() const
{
    return mAction;
}

PXR_NAMESPACE_CLOSE_SCOPE
