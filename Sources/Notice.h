#pragma once

#pragma warning(push, 0)
#include <pxr/base/tf/instantiateType.h>
#include <pxr/base/tf/notice.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/sdf/path.h>
#pragma warning(pop)

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioNotice : public TfNotice
{
public:
    AR_API
    RenderStudioNotice(const SdfPath& path, bool deleted = false, bool appended = false);

    AR_API
    SdfPath GetChangedPrim() const;

    AR_API
    bool WasDeleted() const;

    AR_API
    bool SpecWasCreated() const;

    AR_API
    bool IsValid() const;

private:
    SdfPath mChangedPrim;
    bool mWasDeleted = false;
    bool mSpecWasCreated = false;
    bool mIsValid = true;
};

class RenderStudioLoadingNotice : public TfNotice
{
public:
    enum class Action
    {
        Started,
        Finished
    };

    AR_API
    RenderStudioLoadingNotice(const std::string& name, const std::string& category);

    AR_API
    ~RenderStudioLoadingNotice();

    AR_API
    std::string GetName() const;

    AR_API
    std::string GetCategory() const;

    AR_API
    std::string GetUuid() const;

    AR_API
    Action GetAction() const;

private:
    std::string mName;
    std::string mCategory;
    std::string mUuid;
    Action mAction;
};

PXR_NAMESPACE_CLOSE_SCOPE
