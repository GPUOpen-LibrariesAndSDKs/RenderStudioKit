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

TF_INSTANTIATE_TYPE(RenderStudioNotice::PrimitiveChanged, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioNotice::LiveHistoryStatus, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioNotice::OwnerChanged, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioNotice::WorkspaceState, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioNotice::FileUpdated, TfType::CONCRETE, TF_1_PARENT(TfNotice))
TF_INSTANTIATE_TYPE(RenderStudioNotice::WorkspaceConnectionChanged, TfType::CONCRETE, TF_1_PARENT(TfNotice))

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

RenderStudioNotice::PrimitiveChanged::PrimitiveChanged(const SdfPath& path, bool resynced)
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
RenderStudioNotice::PrimitiveChanged::GetChangedPrim() const
{
    return mChangedPrim;
}

bool
RenderStudioNotice::PrimitiveChanged::WasResynched() const
{
    return mWasResynched;
}

bool
RenderStudioNotice::PrimitiveChanged::IsValid() const
{
    return mIsValid;
}

RenderStudioNotice::LiveHistoryStatus::LiveHistoryStatus(const std::string& name, const std::string& category)
    : mName(name)
    , mCategory(category)
    , mUuid(uuid::generate_uuid_v4())
{
    mAction = RenderStudioNotice::LiveHistoryStatus::Action::Started;
    Send();
}

RenderStudioNotice::LiveHistoryStatus::~LiveHistoryStatus()
{
    mAction = RenderStudioNotice::LiveHistoryStatus::Action::Finished;
    Send();
}

std::string
RenderStudioNotice::LiveHistoryStatus::GetName() const
{
    return mName;
}

std::string
RenderStudioNotice::LiveHistoryStatus::GetCategory() const
{
    return mCategory;
}

std::string
RenderStudioNotice::LiveHistoryStatus::GetUuid() const
{
    return mUuid;
}

RenderStudioNotice::LiveHistoryStatus::Action
RenderStudioNotice::LiveHistoryStatus::GetAction() const
{
    return mAction;
}

RenderStudioNotice::OwnerChanged::OwnerChanged(const SdfPath& path, const std::optional<std::string> owner)
    : mPath(path)
    , mOwner(owner)
{
}

SdfPath
RenderStudioNotice::OwnerChanged::GetPath() const
{
    return mPath;
}

std::optional<std::string>
RenderStudioNotice::OwnerChanged::GetOwner() const
{
    return mOwner;
}

RenderStudioNotice::WorkspaceState::WorkspaceState(RenderStudioNotice::WorkspaceState::State state)
    : mState(state)
{
}

RenderStudioNotice::WorkspaceState::WorkspaceState(const std::string& state)
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

RenderStudioNotice::WorkspaceState::State
RenderStudioNotice::WorkspaceState::GetState() const
{
    return mState;
}

RenderStudioNotice::FileUpdated::FileUpdated(const std::string& path)
    : mPath(path)
{
}

std::string
RenderStudioNotice::FileUpdated::GetPath() const
{
    return mPath;
}

RenderStudioNotice::WorkspaceConnectionChanged::WorkspaceConnectionChanged(bool status)
    : mStatus(status)
{
}

bool
RenderStudioNotice::WorkspaceConnectionChanged::IsConnected() const
{
    return mStatus;
}

PXR_NAMESPACE_CLOSE_SCOPE
