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

boost::json::object SerializeDeltas(SdfLayerHandle layer, const DeltaType& deltas);
std::pair<std::string, DeltaType> DeserializeDeltas(const std::string message);

} // namespace RenderStudioApi