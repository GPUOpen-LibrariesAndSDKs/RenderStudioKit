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

#include "Api.h"

#include "Serialization.h"

namespace
{
struct Helper
{
    template <class T> static void Extract(boost::json::object const& json, T& t, boost::json::string_view key)
    {
        t = boost::json::value_to<T>(json.at(key));
    }

    template <class T>
    static void Extract(boost::json::object const& json, std::optional<T>& t, boost::json::string_view key)
    {
        if (json.if_contains(key))
        {
            t = boost::json::value_to<T>(json.at(key));
        }
    }
};

} // namespace

namespace RenderStudio::API
{
// --- SpecData ---
void
tag_invoke(const value_from_tag&, value& json, const SpecData& v)
{
    object result;
    result["specType"] = boost::json::value_from(v.specType);
    auto& jsonFields = result["fields"].emplace_array();

    for (const auto& field : v.fields)
    {
        boost::json::object jsonField;
        jsonField["key"] = boost::json::value_from(field.first);
        jsonField["value"] = boost::json::value_from(field.second);
        jsonFields.push_back(jsonField);
    }

    json = result;
}

SpecData
tag_invoke(const value_to_tag<SpecData>&, const value& json)
{
    object jsonObject = json.as_object();
    array jsonFields = jsonObject.at("fields").as_array();

    SpecData result;
    result.specType = boost::json::value_to<SdfSpecType>(jsonObject.at("specType"));

    for (const auto& jsonField : jsonFields)
    {
        TfToken key = boost::json::value_to<TfToken>(jsonField.at("key"));
        VtValue value = boost::json::value_to<VtValue>(jsonField.at("value"));
        result.fields.push_back({ key, value });
    }

    return result;
}

// --- DeltaEvent ---
void
tag_invoke(const value_from_tag&, value& json, const DeltaEvent& v)
{
    object result;

    result["layer"] = boost::json::value_from(v.layer);
    result["user"] = boost::json::value_from(v.user);
    if (v.sequence.has_value())
    {
        result["sequence"] = boost::json::value_from(v.sequence.value());
    }

    array& jsonUpdates = result["updates"].emplace_array();

    for (const auto& [path, spec] : v.updates)
    {
        boost::json::object jsonUpdate;
        jsonUpdate["path"] = boost::json::value_from(path);
        jsonUpdate["spec"] = boost::json::value_from(spec.specType);
        auto& jsonFields = jsonUpdate["fields"].emplace_array();

        for (const auto& field : spec.fields)
        {
            boost::json::object jsonField;
            jsonField["key"] = boost::json::value_from(field.first);
            jsonField["value"] = boost::json::value_from(field.second);
            jsonFields.push_back(jsonField);
        }

        jsonUpdates.push_back(jsonUpdate);
    }

    json = result;
}

DeltaEvent
tag_invoke(const value_to_tag<DeltaEvent>&, const value& json)
{
    const boost::json::object& root = json.as_object();
    DeltaEvent result {};

    Helper::Extract(root, result.layer, "layer");
    Helper::Extract(root, result.user, "user");
    Helper::Extract(root, result.sequence, "sequence");

    boost::json::array jsonUpdates = json.at("updates").as_array();
    TfHashMap<SdfPath, SpecData, SdfPath::Hash> updates;

    for (const auto& jsonUpdate : jsonUpdates)
    {
        SdfPath path = boost::json::value_to<SdfPath>(jsonUpdate.at("path"));
        boost::json::array jsonFields = jsonUpdate.at("fields").as_array();

        for (const auto& jsonField : jsonFields)
        {
            TfToken key = boost::json::value_to<TfToken>(jsonField.at("key"));
            VtValue value = boost::json::value_to<VtValue>(jsonField.at("value"));
            result.updates[path].fields.push_back({ key, value });
        }

        result.updates[path].specType = boost::json::value_to<SdfSpecType>(jsonUpdate.at("spec"));
    }

    return result;
}

// --- AcknowledgeEvent ---
void
tag_invoke(const value_from_tag&, value& json, const AcknowledgeEvent& v)
{
    object result;
    result["layer"] = boost::json::value_from(v.layer);
    result["paths"] = boost::json::value_from(v.paths);
    result["sequence"] = boost::json::value_from(v.sequence);
    json = result;
}

AcknowledgeEvent
tag_invoke(const value_to_tag<AcknowledgeEvent>&, const value& json)
{
    boost::json::object root = json.as_object();
    AcknowledgeEvent result;

    Helper::Extract(root, result.layer, "layer");
    Helper::Extract(root, result.paths, "paths");
    Helper::Extract(root, result.sequence, "sequence");

    return result;
}

// --- HistoryEvent ---
void
tag_invoke(const value_from_tag&, value& json, const HistoryEvent& v)
{
    (void)v;
    json = object {};
}

HistoryEvent
tag_invoke(const value_to_tag<HistoryEvent>&, const value& json)
{
    (void)json;
    return {};
}

// --- Event ---
void
tag_invoke(const value_from_tag&, value& json, const Event& v)
{
    object result;
    result["event"] = boost::json::value_from(v.event);

    if (v.event == "Delta::Event")
    {
        result["body"] = boost::json::value_from(std::get<DeltaEvent>(v.body));
    }
    else if (v.event == "Acknowledge::Event")
    {
        result["body"] = boost::json::value_from(std::get<AcknowledgeEvent>(v.body));
    }
    else if (v.event == "History::Event")
    {
        result["body"] = boost::json::value_from(std::get<HistoryEvent>(v.body));
    }
    else
    {
        throw std::runtime_error("JSON event is unsupported");
    }

    json = result;
}

Event
tag_invoke(const value_to_tag<Event>&, const value& json)
{
    Event result;
    object jsonObject = json.as_object();
    std::string jsonEvent = boost::json::value_to<std::string>(jsonObject.at("event"));
    result.event = jsonEvent;

    if (jsonEvent == "Delta::Event")
    {
        result.body = boost::json::value_to<DeltaEvent>(jsonObject.at("body"));
    }
    else if (jsonEvent == "Acknowledge::Event")
    {
        result.body = boost::json::value_to<AcknowledgeEvent>(jsonObject.at("body"));
    }
    else if (jsonEvent == "History::Event")
    {
        result.body = boost::json::value_to<HistoryEvent>(jsonObject.at("body"));
    }
    else
    {
        throw std::runtime_error("JSON event is unsupported");
    }

    return result;
}

} // namespace RenderStudio::API
