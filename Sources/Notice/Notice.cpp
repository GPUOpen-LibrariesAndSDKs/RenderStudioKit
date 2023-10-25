// Copyright 2023 Advanced Micro Devices, Inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Notice.h"

#pragma warning(push, 0)
#include <random>
#include <sstream>
#pragma warning(pop)

#include <Logger/Logger.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_TYPE(RenderStudioPrimitiveNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioLoadingNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioOwnerNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioWorkspaceStateNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioWorkspaceFileNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice))

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

RenderStudioPrimitiveNotice::RenderStudioPrimitiveNotice(const SdfPath& path, bool resynced)
    : mChangedPrim(path)
    , mWasResynched(resynced)
{
    if (!mChangedPrim.IsPrimPath())
    {
        mChangedPrim = mChangedPrim.GetPrimPath();
    }

    if (!mChangedPrim.IsPrimPath() || mChangedPrim.IsAbsoluteRootPath())
    {
        mIsValid = false;
    }
}

SdfPath
RenderStudioPrimitiveNotice::GetChangedPrim() const
{
    return mChangedPrim;
}

bool
RenderStudioPrimitiveNotice::WasResynched() const
{
    return mWasResynched;
}

bool
RenderStudioPrimitiveNotice::IsValid() const
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

RenderStudioOwnerNotice::RenderStudioOwnerNotice(const SdfPath& path, const std::optional<std::string> owner)
    : mPath(path)
    , mOwner(owner)
{
}

SdfPath
RenderStudioOwnerNotice::GetPath() const
{
    return mPath;
}

std::optional<std::string>
RenderStudioOwnerNotice::GetOwner() const
{
    return mOwner;
}

RenderStudioWorkspaceStateNotice::RenderStudioWorkspaceStateNotice(RenderStudioWorkspaceStateNotice::State state)
    : mState(state)
{
}

RenderStudioWorkspaceStateNotice::RenderStudioWorkspaceStateNotice(const std::string& state)
{
    static std::map<std::string, State> states {
        { "idle", State::Idle },
        { "syncing", State::Syncing },
        { "error", State::Error },
    };

    auto it = states.find(state);

    if (it != states.end())
    {
        mState = states.at(state);
    }
    else
    {
        mState = State::Other;
    }
}

RenderStudioWorkspaceStateNotice::State
RenderStudioWorkspaceStateNotice::GetState() const
{
    return mState;
}

RenderStudioWorkspaceFileNotice::RenderStudioWorkspaceFileNotice(const std::string& path)
    : mPath(path)
{
}

std::string
RenderStudioWorkspaceFileNotice::GetPath() const
{
    return mPath;
}

PXR_NAMESPACE_CLOSE_SCOPE
