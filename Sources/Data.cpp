#include "Data.h"

#include <iostream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/json/src.hpp>

#include <pxr/pxr.h>
#include <pxr/base/trace/trace.h>
#include <pxr/base/work/utils.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerStateDelegate.h>

#include "Serialization/Serialization.h"
#include "Logger/Logger.h"

PXR_NAMESPACE_OPEN_SCOPE

RenderStudioData::RenderStudioData()
{
    std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());

    mWebsocketClient = std::make_shared<RenderStudio::Networking::WebsocketClient>(
        [this, uuid](const std::string& message)
        {
            _HashTable deltas;

            try
            {
                boost::json::value jsonRoot = boost::json::parse(message);
                boost::json::array jsonUpdates = jsonRoot.at("updates").as_array();

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
                }
            }
            catch (const std::exception& ex)
            {
                LOG_WARNING << ex.what() << " (" << message << ")";
            }

            std::unique_lock<std::mutex> lock(mRemoteMutex);
            mRemoteDeltas = deltas;
        }
    );

    mWebsocketClient->Connect({ "127.0.0.1", "8000", "/live/" + uuid });
    LOG_FATAL << "RenderStudioData created";
}

RenderStudioData::~RenderStudioData()
{
    // Clear out mData in parallel, since it can get big.
    WorkSwapDestroyAsync(mData);
    mShouldStop = true;
    mWebsocketClient->Disconnect();
}

void RenderStudioData::ProcessLiveUpdates(SdfLayerHandle& layer)
{
    // Send updates
    boost::json::object jsonRoot;

    jsonRoot["layer"] = boost::json::value_from(layer);
    auto& jsonUpdates = jsonRoot["updates"].emplace_array();

    for (auto& [path, spec] : mLocalDeltas)
    {
        try
        {
            boost::json::object jsonUpdate;

            jsonUpdate["path"] = boost::json::value_from(path);
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
        catch (const std::exception& ex)
        {
            LOG_WARNING << ex.what();
        }
    }

    if (!mLocalDeltas.empty())
    {
        mWebsocketClient->SendMessageString(boost::json::serialize(jsonRoot));
    }

    // Receive updates
    SdfChangeBlock block;

    mRemoteDataRequested = true;
    std::unique_lock<std::mutex> lock(mRemoteMutex);
    auto copy = mRemoteDeltas;
    mRemoteDeltas.clear();
    lock.unlock();

    for (auto& [path, spec] : copy)
    {
        for (const auto& field : spec.fields)
        {
            layer->GetStateDelegate()->SetField(path, field.first, field.second);
        }
    }

    mLocalDeltas.clear();
}

bool
RenderStudioData::StreamsData() const
{
    return true;
}

bool
RenderStudioData::HasSpec(const SdfPath &path) const
{
    return mData.find(path) != mData.end();
}

void
RenderStudioData::EraseSpec(const SdfPath &path)
{
    _HashTable::iterator i = mData.find(path);
    if (!TF_VERIFY(i != mData.end(),
                   "No spec to erase at <%s>", path.GetText())) {
        return;
    }
    mData.erase(i);
}

void
RenderStudioData::MoveSpec(const SdfPath &oldPath,
                  const SdfPath &newPath)
{
    _HashTable::iterator old = mData.find(oldPath);
    if (!TF_VERIFY(old != mData.end(),
            "No spec to move at <%s>", oldPath.GetString().c_str())) {
        return;
    }
    bool inserted = mData.insert(std::make_pair(newPath, old->second)).second;
    if (!TF_VERIFY(inserted)) {
        return;
    }
    mData.erase(old);
}

SdfSpecType
RenderStudioData::GetSpecType(const SdfPath &path) const
{
    _HashTable::const_iterator i = mData.find(path);
    if (i == mData.end()) {
        return SdfSpecTypeUnknown;
    }
    return i->second.specType;
}

void
RenderStudioData::CreateSpec(const SdfPath &path, SdfSpecType specType)
{
    if (!TF_VERIFY(specType != SdfSpecTypeUnknown)) {
        return;
    }
    mData[path].specType = specType;
}

void
RenderStudioData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    TF_FOR_ALL(it, mData) {
        if (!visitor->VisitSpec(*this, it->first)) {
            break;
        }
    }
}

bool 
RenderStudioData::Has(const SdfPath &path, const TfToken &field,
             SdfAbstractDataValue* value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(path, field)) {
        if (value) {
            return value->StoreValue(*fieldValue);
        }
        return true;
    }
    return false;
}

bool 
RenderStudioData::Has(const SdfPath &path, const TfToken & field, 
             VtValue *value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(path, field)) {
        if (value) {
            *value = *fieldValue;
        }
        return true;
    }
    return false;
}

bool
RenderStudioData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    SdfAbstractDataValue *value, SdfSpecType *specType) const
{
    if (VtValue const *v =
        _GetSpecTypeAndFieldValue(path, fieldName, specType)) {
        return !value || value->StoreValue(*v);
    }
    return false;
}

bool
RenderStudioData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    VtValue *value, SdfSpecType *specType) const
{
    if (VtValue const *v =
        _GetSpecTypeAndFieldValue(path, fieldName, specType)) {
        if (value) {
            *value = *v;
        }
        return true;
    }
    return false;
}

const VtValue*
RenderStudioData::_GetSpecTypeAndFieldValue(const SdfPath& path,
                                   const TfToken& field,
                                   SdfSpecType* specType) const
{
    _HashTable::const_iterator i = mData.find(path);
    if (i == mData.end()) {
        *specType = SdfSpecTypeUnknown;
    }
    else {
        const _SpecData &spec = i->second;
        *specType = spec.specType;
        for (auto const &f: spec.fields) {
            if (f.first == field) {
                return &f.second;
            }
        }
    }
    return nullptr;
}

const VtValue* 
RenderStudioData::_GetFieldValue(const SdfPath &path,
                        const TfToken &field) const
{
    _HashTable::const_iterator i = mData.find(path);
    if (i != mData.end()) {
        const _SpecData & spec = i->second;
        for (auto const &f: spec.fields) {
            if (f.first == field) {
                return &f.second;
            }
        }
    }
    return nullptr;
}

VtValue*
RenderStudioData::_GetMutableFieldValue(const SdfPath &path,
                               const TfToken &field)
{
    _HashTable::iterator i = mData.find(path);
    if (i != mData.end()) {
        _SpecData &spec = i->second;
        for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
            if (spec.fields[j].first == field) {
                return &spec.fields[j].second;
            }
        }
    }
    return NULL;
}
 
VtValue
RenderStudioData::Get(const SdfPath &path, const TfToken & field) const
{
    if (const VtValue *value = _GetFieldValue(path, field)) {
        return *value;
    }
    return VtValue();
}

void 
RenderStudioData::Set(const SdfPath &path, const TfToken & field, 
             const VtValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "RenderStudioData::Set");

    if (value.IsEmpty()) {
        Erase(path, field);
        return;
    }

    // Default
    VtValue* newValue = _GetOrCreateFieldValue(path, field);

    if (newValue) {
        *newValue = value;
    }

    // Delta
    VtValue* newValueDelta = _GetOrCreateFieldValueDelta(path, field);
    if (newValueDelta) {
        *newValueDelta = value;
    }
}

void 
RenderStudioData::Set(const SdfPath &path, const TfToken &field, 
             const SdfAbstractDataConstValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "RenderStudioData::Set");

    // Default
    VtValue* newValue = _GetOrCreateFieldValue(path, field);
    if (newValue) {
        value.GetValue(newValue);
    }

    // Delta
    VtValue* newValueDelta = _GetOrCreateFieldValueDelta(path, field);
    if (newValueDelta) {
        value.GetValue(newValueDelta);
    }
}

VtValue* 
RenderStudioData::_GetOrCreateFieldValue(const SdfPath &path,
                                const TfToken &field)
{
    _HashTable::iterator i = mData.find(path);
    if (!TF_VERIFY(i != mData.end(),
                   "No spec at <%s> when trying to set field '%s'",
                   path.GetText(), field.GetText())) {
        return nullptr;
    }

    _SpecData &spec = i->second;
    for (auto &f: spec.fields) {
        if (f.first == field) {
            return &f.second;
        }
    }

    spec.fields.emplace_back(std::piecewise_construct,
                             std::forward_as_tuple(field),
                             std::forward_as_tuple());

    return &spec.fields.back().second;
}

VtValue*
RenderStudioData::_GetOrCreateFieldValueDelta(const SdfPath& path,
    const TfToken& field)
{
    // Apply spec type from mData to _deltas
    if (mLocalDeltas.find(path) == mLocalDeltas.end())
    {
        _HashTable::iterator i = mData.find(path);

        if (!TF_VERIFY(i != mData.end(),
            "No spec at <%s> when trying to set field '%s'",
            path.GetText(), field.GetText())) {
            return nullptr;
        }

        mLocalDeltas[path].specType = i->second.specType;
    }

    _HashTable::iterator i = mLocalDeltas.find(path);
    _SpecData& spec = i->second;

    for (auto& f : spec.fields) {
        if (f.first == field) {
            return &f.second;
        }
    }

    spec.fields.emplace_back(std::piecewise_construct,
        std::forward_as_tuple(field),
        std::forward_as_tuple());

    return &spec.fields.back().second;
}

void 
RenderStudioData::Erase(const SdfPath &path, const TfToken & field)
{
    _HashTable::iterator i = mData.find(path);
    if (i == mData.end()) {
        return;
    }
    
    _SpecData &spec = i->second;
    for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
        if (spec.fields[j].first == field) {
            spec.fields.erase(spec.fields.begin()+j);
            return;
        }
    }
}

std::vector<TfToken>
RenderStudioData::List(const SdfPath &path) const
{
    std::vector<TfToken> names;
    _HashTable::const_iterator i = mData.find(path);
    if (i != mData.end()) {
        const _SpecData & spec = i->second;

        const size_t numFields = spec.fields.size();
        names.resize(numFields);
        for (size_t j=0; j != numFields; ++j) {
            names[j] = spec.fields[j].first;
        }
    }
    return names;
}


////////////////////////////////////////////////////////////////////////
// This is a basic prototype implementation of the time-sampling API
// for in-memory, non cached presto layers.

std::set<double>
RenderStudioData::ListAllTimeSamples() const
{
    // Use a set to determine unique times.
    std::set<double> times;

    TF_FOR_ALL(i, mData) {
        std::set<double> timesForPath = ListTimeSamplesForPath(i->first);
        times.insert(timesForPath.begin(), timesForPath.end());
    }

    return times;
}

std::set<double>
RenderStudioData::ListTimeSamplesForPath(const SdfPath &path) const
{
    std::set<double> times;
    
    VtValue value = Get(path, SdfDataTokens->TimeSamples);
    if (value.IsHolding<SdfTimeSampleMap>()) {
        const SdfTimeSampleMap & timeSampleMap =
            value.UncheckedGet<SdfTimeSampleMap>();
        TF_FOR_ALL(j, timeSampleMap) {
            times.insert(j->first);
        }
    }

    return times;
}

template <class Container, class GetTime>
static bool
_GetBracketingTimeSamplesImpl(
    const Container &samples, const GetTime &getTime,
    const double time, double* tLower, double* tUpper)
{
    if (samples.empty()) {
        // No samples.
        return false;
    } else if (time <= getTime(*samples.begin())) {
        // Time is at-or-before the first sample.
        *tLower = *tUpper = getTime(*samples.begin());
    } else if (time >= getTime(*samples.rbegin())) {
        // Time is at-or-after the last sample.
        *tLower = *tUpper = getTime(*samples.rbegin());
    } else {
        auto iter = samples.lower_bound(time);
        if (getTime(*iter) == time) {
            // Time is exactly on a sample.
            *tLower = *tUpper = getTime(*iter);
        } else {
            // Time is in-between samples; return the bracketing times.
            *tUpper = getTime(*iter);
            --iter;
            *tLower = getTime(*iter);
        }
    }
    return true;
}

static bool
_GetBracketingTimeSamples(const std::set<double> &samples, double time,
                          double *tLower, double *tUpper)
{
    return _GetBracketingTimeSamplesImpl(samples, [](double t) { return t; },
                                         time, tLower, tUpper);
}

static bool
_GetBracketingTimeSamples(const SdfTimeSampleMap &samples, double time,
                          double *tLower, double *tUpper)
{
    return _GetBracketingTimeSamplesImpl(
        samples, [](SdfTimeSampleMap::value_type const &p) { return p.first; },
        time, tLower, tUpper);
}

bool
RenderStudioData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
    return _GetBracketingTimeSamples(
        ListAllTimeSamples(), time, tLower, tUpper);
}

size_t
RenderStudioData::GetNumTimeSamplesForPath(const SdfPath &path) const
{
    if (const VtValue *fval = _GetFieldValue(path, SdfDataTokens->TimeSamples)) {
        if (fval->IsHolding<SdfTimeSampleMap>()) {
            return fval->UncheckedGet<SdfTimeSampleMap>().size();
        }
    }
    return 0;
}

bool
RenderStudioData::GetBracketingTimeSamplesForPath(
    const SdfPath &path, double time,
    double* tLower, double* tUpper) const
{
    const VtValue *fval = _GetFieldValue(path, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>()) {
        auto const &tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        return _GetBracketingTimeSamples(tsmap, time, tLower, tUpper);
    }
    return false;
}

bool
RenderStudioData::QueryTimeSample(const SdfPath &path, double time, 
                         VtValue *value) const
{
    const VtValue *fval = _GetFieldValue(path, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>()) {
        auto const &tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        auto iter = tsmap.find(time);
        if (iter != tsmap.end()) {
            if (value)
                *value = iter->second;
            return true;
        }
    }
    return false;
}

bool 
RenderStudioData::QueryTimeSample(const SdfPath &path, double time,
                         SdfAbstractDataValue* value) const
{ 
    const VtValue *fval = _GetFieldValue(path, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>()) {
        auto const &tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        auto iter = tsmap.find(time);
        if (iter != tsmap.end()) {
            return !value || value->StoreValue(iter->second);
        }
    }
    return false;
}

void
RenderStudioData::SetTimeSample(const SdfPath &path, double time, 
                       const VtValue& value)
{
    if (value.IsEmpty()) {
        EraseTimeSample(path, time);
        return;
    }

    SdfTimeSampleMap newSamples;

    // Attempt to get a pointer to an existing timeSamples field.
    VtValue *fieldValue =
        _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);

    // If we have one, swap it out so we can modify it.
    if (fieldValue && fieldValue->IsHolding<SdfTimeSampleMap>()) {
        fieldValue->UncheckedSwap(newSamples);
    }
    
    // Insert or overwrite into newSamples.
    newSamples[time] = value;

    // Set back into the field.
    if (fieldValue) {
        fieldValue->Swap(newSamples);
    } else {
        Set(path, SdfDataTokens->TimeSamples, VtValue::Take(newSamples));
    }
}

void
RenderStudioData::EraseTimeSample(const SdfPath &path, double time)
{
    SdfTimeSampleMap newSamples;

    // Attempt to get a pointer to an existing timeSamples field.
    VtValue *fieldValue =
        _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);

    // If we have one, swap it out so we can modify it.  If we do not have one,
    // there's nothing to erase so we're done.
    if (fieldValue && fieldValue->IsHolding<SdfTimeSampleMap>()) {
        fieldValue->UncheckedSwap(newSamples);
    } else {
        return;
    }
    
    // Erase from newSamples.
    newSamples.erase(time);

    // Check to see if the result is empty.  In that case we remove the field.
    if (newSamples.empty()) {
        Erase(path, SdfDataTokens->TimeSamples);
    } else {
        fieldValue->UncheckedSwap(newSamples);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
