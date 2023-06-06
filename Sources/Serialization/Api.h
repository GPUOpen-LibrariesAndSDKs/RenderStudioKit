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

namespace RenderStudioApi
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
    TfHashMap<SdfPath, SpecData, SdfPath::Hash> updates;
};

void tag_invoke(const value_from_tag&, value& json, const DeltaEvent& v);
DeltaEvent tag_invoke(const value_to_tag<DeltaEvent>&, const value& json);

struct AcknowledgeEvent
{
    std::string layer;
    std::vector<SdfPath> paths;
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

typedef std::pair<TfToken, VtValue> _FieldValuePair;
struct _SpecData
{
    _SpecData()
        : specType(SdfSpecTypeUnknown)
    {
    }

    SdfSpecType specType;
    std::vector<_FieldValuePair> fields;
};

typedef SdfPath _Key;
typedef SdfPath::Hash _KeyHash;
typedef TfHashMap<_Key, _SpecData, _KeyHash> _HashTable;
using DeltaType = _HashTable;

boost::json::object SerializeDeltas(SdfLayerHandle layer, const DeltaType& deltas, const std::string& user);
std::tuple<std::string, DeltaType, std::size_t> DeserializeDeltas(const std::string message);

} // namespace RenderStudioApi