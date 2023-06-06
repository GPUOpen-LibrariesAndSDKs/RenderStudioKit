#include "Api.h"

#include "Serialization.h"

namespace RenderStudioApi
{

boost::json::object
SerializeDeltas(SdfLayerHandle layer, const DeltaType& deltas, const std::string& user)
{
    boost::json::object jsonRoot;
    jsonRoot["layer"] = boost::json::value_from(layer);
    jsonRoot["user"] = boost::json::value_from(user);
    auto& jsonUpdates = jsonRoot["updates"].emplace_array();

    for (auto& [path, spec] : deltas)
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

    return jsonRoot;
}

std::tuple<std::string, DeltaType, std::size_t>
DeserializeDeltas(const std::string message)
{
    DeltaType deltas;

    boost::json::value jsonRoot = boost::json::parse(message);
    std::size_t sequence = boost::json::value_to<std::size_t>(jsonRoot.at("sequence"));
    std::string layerName = boost::json::value_to<std::string>(jsonRoot.at("layer"));

    if (jsonRoot.as_object().if_contains("updates"))
    {
        boost::json::array jsonUpdates = jsonRoot.at("updates").as_array();
        for (const auto& jsonUpdate : jsonUpdates)
        {
            SdfPath path = boost::json::value_to<SdfPath>(jsonUpdate.at("path"));

            if (jsonUpdate.as_object().if_contains("fields"))
            {
                boost::json::array jsonFields = jsonUpdate.at("fields").as_array();

                for (const auto& jsonField : jsonFields)
                {
                    TfToken key = boost::json::value_to<TfToken>(jsonField.at("key"));
                    VtValue value = boost::json::value_to<VtValue>(jsonField.at("value"));
                    deltas[path].fields.push_back({ key, value });
                }
            }

            deltas[path].specType = boost::json::value_to<SdfSpecType>(jsonUpdate.at("spec"));
        }
    }

    return { layerName, deltas, sequence };
}

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
    result["updates"] = boost::json::value_from(v.updates);
    json = result;
}

DeltaEvent
tag_invoke(const value_to_tag<DeltaEvent>&, const value& json)
{
    return DeltaEvent {
        boost::json::value_to<std::string>(json.at("layer")), boost::json::value_to<std::string>(json.at("user")),
        /*boost::json::value_to<TfHashMap<SdfPath, SpecData, SdfPath::Hash>>(json.at("updates")),*/
    };
}

// --- AcknowledgeEvent ---
void
tag_invoke(const value_from_tag&, value& json, const AcknowledgeEvent& v)
{
    object result;
    result["layer"] = boost::json::value_from(v.layer);
    result["paths"] = boost::json::value_from(v.paths);
    json = result;
}

AcknowledgeEvent
tag_invoke(const value_to_tag<AcknowledgeEvent>&, const value& json)
{
    return AcknowledgeEvent { boost::json::value_to<std::string>(json.at("layer")),
                              boost::json::value_to<std::vector<SdfPath>>(json.at("paths")) };
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

} // namespace RenderStudioApi
