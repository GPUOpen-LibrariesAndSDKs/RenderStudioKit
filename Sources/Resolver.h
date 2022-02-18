#ifndef USD_REMOTE_ASSET_RESOLVER_H
#define USD_REMOTE_ASSET_RESOLVER_H

#include <pxr/pxr.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/defaultResolver.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "WebSocketClient.h"

PXR_NAMESPACE_OPEN_SCOPE

class WebUsdAssetResolver : public ArDefaultResolver
{
public:
    AR_API
    WebUsdAssetResolver();

    AR_API
    virtual ~WebUsdAssetResolver();

    AR_API
    static void SetRemoteServerAddress(std::string protocol, std::string host, std::uint32_t port);

    AR_API
    virtual ArResolvedPath _Resolve(const std::string& path) override;

protected:
    AR_API
    virtual std::shared_ptr<ArAsset> _OpenAsset(const ArResolvedPath& resolvedPath) override;

    AR_API
    virtual VtValue _GetModificationTimestamp(const std::string& path, const ArResolvedPath& resolvedPath) override;

private:
    std::string HttpGetRequest(const std::string& asset) const;
    std::string WebSocketRequest(const std::string& asset) const;

    mutable boost::uuids::random_generator mUuidGenerator;
    mutable websocket_endpoint mLiveServerEndpoint;

    static inline std::string mProtocol;
    static inline std::string mHost;
    static inline std::uint32_t mPort = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
