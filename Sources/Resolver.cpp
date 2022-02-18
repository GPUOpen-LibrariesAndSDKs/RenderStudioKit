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

#include <pxr/usd/ar/defineResolver.h>
#include <Logger.hpp>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(WebUsdAssetResolver, ArResolver);

WebUsdAssetResolver::WebUsdAssetResolver()
{
    Logger::Info("Created");
}

WebUsdAssetResolver::~WebUsdAssetResolver()
{
    Logger::Info("Destroyed");
}

void
WebUsdAssetResolver::SetRemoteServerAddress(std::string protocol, std::string host, std::uint32_t port) {
    mHost = host;
    mPort = port;
    mProtocol = protocol;
}

ArResolvedPath
WebUsdAssetResolver::_Resolve(const std::string& path)
{
    const std::regex protocol("^webusd:/[/]?");
    std::stringstream result;
    std::regex_replace(std::ostream_iterator<char>(result), path.begin(), path.end(), protocol, "");
    return ArResolvedPath(result.str());
}

std::shared_ptr<ArAsset>
WebUsdAssetResolver::_OpenAsset(const ArResolvedPath& resolvedPath)
{
    std::string assetData;
    
    if (mProtocol == "http") {
        assetData = HttpGetRequest(resolvedPath);
    } else if (mProtocol == "ws") {
        assetData = WebSocketRequest(resolvedPath);
    }

    std::size_t assetSize = assetData.size();

    const char* data = new char[assetSize];
    std::strncpy(const_cast<char*>(data), assetData.c_str(), assetSize);
    auto assetPtr = std::shared_ptr<const char>(data);
    
    return std::shared_ptr<ArAsset>(new WebUsdAsset(assetPtr, assetSize));
}

VtValue
WebUsdAssetResolver::_GetModificationTimestamp(
    const std::string& path,
    const ArResolvedPath& resolvedPath)
{
    return VtValue(0);
}

std::string 
WebUsdAssetResolver::HttpGetRequest(const std::string& asset) const 
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
        return "";
    }
}

std::string 
WebUsdAssetResolver::WebSocketRequest(const std::string& asset) const 
{
    if (mProtocol == "ws" && !mLiveServerEndpoint.IsConnected()) {
        std::string url = mProtocol + "://" + mHost + ":" + std::to_string(mPort);
        mLiveServerEndpoint.Connect(url);
        Logger::Info("Connected to live server");
    }

    std::string tag = boost::uuids::to_string(mUuidGenerator());

    nlohmann::json request = {
        {"event", "StageLayerGet"},
        {"body", {
            {"tag", tag},
            {"path", asset}
        }}
    };

    mLiveServerEndpoint.send(request.dump());
    nlohmann::json response = mLiveServerEndpoint.wait_for_message_tag(tag).get();

    return response.get<std::string>();
}

PXR_NAMESPACE_CLOSE_SCOPE
