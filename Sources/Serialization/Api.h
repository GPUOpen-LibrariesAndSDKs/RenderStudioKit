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

#pragma once

#pragma warning(push, 0)
#include <utility>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/abstractData.h>
#include <pxr/usd/sdf/api.h>
#include <pxr/usd/sdf/data.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <boost/json.hpp>
#pragma warning(pop)

namespace RenderStudio::API
{

PXR_NAMESPACE_USING_DIRECTIVE
using namespace boost::json;

struct SpecData
{
    SdfSpecType specType = SdfSpecTypeUnknown;
    std::vector<std::pair<TfToken, VtValue>> fields;
};

void tag_invoke(const value_from_tag&, value& json, const SpecData& v);
SpecData tag_invoke(const value_to_tag<SpecData>&, const value& json);

struct DeltaEvent
{
    std::string layer;
    std::string user;
    std::optional<std::size_t> sequence;
    TfHashMap<SdfPath, SpecData, SdfPath::Hash> updates;
};

void tag_invoke(const value_from_tag&, value& json, const DeltaEvent& v);
DeltaEvent tag_invoke(const value_to_tag<DeltaEvent>&, const value& json);

struct AcknowledgeEvent
{
    std::string layer;
    std::vector<SdfPath> paths;
    std::size_t sequence = 0;
};

void tag_invoke(const value_from_tag&, value& json, const AcknowledgeEvent& v);
AcknowledgeEvent tag_invoke(const value_to_tag<AcknowledgeEvent>&, const value& json);

struct HistoryEvent
{
};

void tag_invoke(const value_from_tag&, value& json, const HistoryEvent& v);
HistoryEvent tag_invoke(const value_to_tag<HistoryEvent>&, const value& json);

struct Event
{
    std::string event;
    std::variant<DeltaEvent, AcknowledgeEvent, HistoryEvent> body;
};

void tag_invoke(const value_from_tag&, value& json, const Event& v);
Event tag_invoke(const value_to_tag<Event>&, const value& json);

} // namespace RenderStudio::API
