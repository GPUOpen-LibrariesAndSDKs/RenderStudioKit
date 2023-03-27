#pragma once

#pragma warning(push, 0)
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
#pragma warning(pop)

#include "Serialization/Api.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(RenderStudioData);

class RenderStudioFileFormat;

class RenderStudioData : public SdfAbstractData
{
public:
    RenderStudioData();

    SDF_API
    virtual ~RenderStudioData();

    /// SdfAbstractData overrides
    SDF_API
    virtual bool StreamsData() const;

    SDF_API
    virtual void CreateSpec(const SdfPath& path, SdfSpecType specType);
    SDF_API
    virtual bool HasSpec(const SdfPath& path) const;
    SDF_API
    virtual void EraseSpec(const SdfPath& path);
    SDF_API
    virtual void MoveSpec(const SdfPath& oldPath, const SdfPath& newPath);
    SDF_API
    virtual SdfSpecType GetSpecType(const SdfPath& path) const;

    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken& fieldName, SdfAbstractDataValue* value) const;
    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken& fieldName, VtValue* value = NULL) const;
    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath& path, const TfToken& fieldName, SdfAbstractDataValue* value, SdfSpecType* specType)
        const;

    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath& path, const TfToken& fieldName, VtValue* value, SdfSpecType* specType) const;

    SDF_API
    virtual VtValue Get(const SdfPath& path, const TfToken& fieldName) const;
    SDF_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName, const VtValue& value);
    SDF_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName, const SdfAbstractDataConstValue& value);
    SDF_API
    virtual void Erase(const SdfPath& path, const TfToken& fieldName);
    SDF_API
    virtual std::vector<TfToken> List(const SdfPath& path) const;

    SDF_API
    virtual std::set<double> ListAllTimeSamples() const;

    SDF_API
    virtual std::set<double> ListTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    virtual bool GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    SDF_API
    virtual size_t GetNumTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath& path, double time, double* tLower, double* tUpper) const;

    SDF_API
    virtual bool QueryTimeSample(const SdfPath& path, double time, SdfAbstractDataValue* optionalValue) const;
    SDF_API
    virtual bool QueryTimeSample(const SdfPath& path, double time, VtValue* value) const;

    SDF_API
    virtual void SetTimeSample(const SdfPath& path, double time, const VtValue& value);

    SDF_API
    virtual void EraseTimeSample(const SdfPath& path, double time);

protected:
    // SdfAbstractData overrides
    SDF_API
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

private:
    void ApplyRemoteDeltas(SdfLayerHandle& layer);
    void AddRemoteSequence(SdfLayerHandle& layer, const RenderStudioApi::DeltaType& deltas, std::size_t sequence);
    RenderStudioApi::DeltaType FetchLocalDeltas();

    const VtValue* _GetSpecTypeAndFieldValue(const SdfPath& path, const TfToken& field, SdfSpecType* specType) const;

    const VtValue* _GetFieldValue(const SdfPath& path, const TfToken& field) const;

    VtValue* _GetMutableFieldValue(const SdfPath& path, const TfToken& field);

    VtValue* _GetOrCreateFieldValue(const SdfPath& path, const TfToken& field);

    VtValue* _GetOrCreateFieldValueDelta(const SdfPath& path, const TfToken& field);

private:
    friend class RenderStudioFileFormat;
    using _HashTable = RenderStudioApi::_HashTable;
    using _SpecData = RenderStudioApi::_SpecData;

    _HashTable mData;
    _HashTable mLocalDeltas;
    std::mutex mRemoteMutex;
    std::size_t mLatestAppliedSequence = 0;
    std::map<std::size_t, RenderStudioApi::DeltaType> mRemoteDeltasQueue;
    bool mFirstFetch = true;
};

PXR_NAMESPACE_CLOSE_SCOPE
