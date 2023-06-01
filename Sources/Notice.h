#pragma once

#pragma warning(push, 0)
#include <optional>

#include <pxr/base/tf/instantiateType.h>
#include <pxr/base/tf/notice.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/sdf/path.h>
#pragma warning(pop)

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioPrimitiveNotice : public TfNotice
{
public:
    AR_API
    RenderStudioPrimitiveNotice(const SdfPath& path, bool resynched = false);

    AR_API
    SdfPath GetChangedPrim() const;

    AR_API
    bool WasResynched() const;

    AR_API
    bool IsValid() const;

private:
    SdfPath mChangedPrim;
    bool mWasResynched = false;
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

class RenderStudioOwnerNotice : public TfNotice
{
public:
    AR_API
    RenderStudioOwnerNotice(const SdfPath& path, const std::optional<std::string> owner);

    AR_API
    SdfPath GetPath() const;

    AR_API
    std::optional<std::string> GetOwner() const;

private:
    SdfPath mPath;
    std::optional<std::string> mOwner;
};

PXR_NAMESPACE_CLOSE_SCOPE
