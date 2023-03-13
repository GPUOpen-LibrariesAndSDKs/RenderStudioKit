#include "Api.h"

#include "Serialization.h"

namespace RenderStudioApi
{

boost::json::object
SerializeDeltas(SdfLayerHandle layer, const DeltaType& deltas)
{
    boost::json::object jsonRoot;
    jsonRoot["layer"] = boost::json::value_from(layer);
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

std::pair<std::string, DeltaType>
DeserializeDeltas(const std::string message)
{
    boost::json::value jsonRoot = boost::json::parse(message);
    boost::json::array jsonUpdates = jsonRoot.at("updates").as_array();
    std::string layerName = boost::json::value_to<std::string>(jsonRoot.at("layer"));

    DeltaType deltas;

    for (const auto& jsonUpdate : jsonUpdates)
    {
        SdfPath path = boost::json::value_to<SdfPath>(jsonUpdate.at("path"));
        boost::json::array jsonFields = jsonUpdate.at("fields").as_array();

        for (const auto& jsonField : jsonFields)
        {
            TfToken key = boost::json::value_to<TfToken>(jsonField.at("key"));
            VtValue value = boost::json::value_to<VtValue>(jsonField.at("value"));
            deltas[path].fields.push_back({ key, value });
        }

        deltas[path].specType = boost::json::value_to<SdfSpecType>(jsonUpdate.at("spec"));
    }

    return { layerName, deltas };
}

} // namespace RenderStudioApi
