#include "Resolver.h"
#include "Asset.h"

#include <iostream>
#include <regex>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <Logger.hpp>
#include <pxr/usd/ar/defineResolver.h>
#include <pxr/base/tf/diagnostic.h>
#include <base64.hpp>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(RenderStudioResolver, ArResolver);

RenderStudioResolver::RenderStudioResolver()
{
    Logger::Info("Created");
}

RenderStudioResolver::~RenderStudioResolver()
{
    Logger::Info("Destroyed");
}

void
RenderStudioResolver::SetRemoteServerAddress(std::string protocol, std::string host, std::uint32_t port) {
    Logger::Info("Set remote server address to {}://{}:{}", protocol, host, port);
    mHost = host;
    mPort = port;
    mProtocol = protocol;
}

ArResolvedPath
RenderStudioResolver::_Resolve(const std::string& path) const
{
    // Do not resolve here and do that in _OpenAsset()
    // That's because asset resolver config is broken and all the assets (even without webusd:// prefix came here)
    // So we need to resolve all the assets here manually
    return ArResolvedPath(path);
}

std::shared_ptr<ArAsset>
RenderStudioResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const
{
    // Resolve here
    std::string path = resolvedPath.GetPathString();
    if (path.find("webusd:/") == std::string::npos) {
        return ArDefaultResolver::_OpenAsset(resolvedPath);
    }

    std::string assetData;
    
    // Download asset
    if (mProtocol == "http") {
        // Remove webusd:// prefix
        const std::regex protocol("^webusd:/[/]?");
        std::stringstream result;
        std::regex_replace(std::ostream_iterator<char>(result), path.begin(), path.end(), protocol, "");
        std::string resolvedString = result.str();
        assetData = HttpGetRequest(resolvedString);
    } else if (mProtocol == "ws") {
        assetData = WebSocketRequest(path);
    }

    // Prepare payload
    std::size_t assetSize = assetData.size();
    const char* data = new char[assetSize];
    std::strncpy(const_cast<char*>(data), assetData.c_str(), assetSize);
    auto assetPtr = std::shared_ptr<const char>(data);
    
    return std::shared_ptr<ArAsset>(new RenderStudioAsset(assetPtr, assetSize));
}

ArTimestamp
RenderStudioResolver::_GetModificationTimestamp(
    const std::string& path,
    const ArResolvedPath& resolvedPath) const
{
    return ArTimestamp(0);
}

std::string 
RenderStudioResolver::HttpGetRequest(const std::string& asset) const 
{
    namespace beast = boost::beast;     // from <boost/beast.hpp>
    namespace http = beast::http;       // from <boost/beast/http.hpp>
    namespace net = boost::asio;        // from <boost/asio.hpp>
    using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

    try
    {
        std::string target = "/api/layers/?path=" + asset;
        boost::replace_all(target, " ", "%20");

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(mHost, std::to_string(mPort));
        
        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, mHost);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response_parser<http::dynamic_body> parser;
        parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

        // Receive the HTTP response
        http::read(stream, buffer, parser);

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // Not_connected happens sometimes
        // so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected) {
            throw beast::system_error{ec};
        }

        return boost::beast::buffers_to_string(parser.get().body().data());
    }
    catch(std::exception const& e)
    {
        Logger::Error(e.what());
        throw std::runtime_error(std::string(e.what()) + ", host: " + mHost + ", port: " + std::to_string(mPort));
        return "";
    }
}

std::string 
RenderStudioResolver::WebSocketRequest(const std::string& asset) const 
{
    const RenderStudioResolverContext* context =_GetCurrentContextObject<RenderStudioResolverContext>();

    if (context == nullptr) {
        Logger::Error("RenderStudioResolverContext must be bound to RenderStudioResolver");
        return "";
    }

    std::string currentResolverUuid = context->GetValue();
    websocket_endpoint* liveEndpoint = nullptr;

    if (mLiveServerEndpoints.find(currentResolverUuid) == mLiveServerEndpoints.end()) {
        // Place to map
        auto endpointPtr = std::make_unique<websocket_endpoint>();
        mLiveServerEndpoints.emplace(currentResolverUuid, std::move(endpointPtr));

        // Get raw pointer
        liveEndpoint = mLiveServerEndpoints[currentResolverUuid].get();
        
        // Connect
        std::string url = mProtocol + "://" + mHost + ":" + std::to_string(mPort);
        liveEndpoint->Connect(url).wait();
        
        Logger::Info("Connected to live server");
    } else {
        liveEndpoint = mLiveServerEndpoints[currentResolverUuid].get();
    }

    std::string tag = boost::uuids::to_string(mUuidGenerator());

    nlohmann::json request = {
        {"event", "StageLayerGet"},
        {"body", {
            {"tag", tag},
            {"path", asset}
        }}
    };

    liveEndpoint->send(request.dump());

    nlohmann::json response = liveEndpoint->wait_for_message_tag(tag).get();
    std::string layer = base64_decode(response["body"]["data"].get<std::string>());
    
    return layer;
}

PXR_NAMESPACE_CLOSE_SCOPE
