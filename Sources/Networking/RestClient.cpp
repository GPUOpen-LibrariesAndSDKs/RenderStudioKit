#include "RestClient.h"

#pragma warning(push, 0)
#include <set>
#ifdef _WIN32
#include <wincrypt.h>
#elif __linux__
#error "Linux certificate loading not implemented"
#endif
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#pragma warning(pop)

#include <Logger/Logger.h>

namespace RenderStudio::Networking
{

void
AddWindowsRootCertificatesFromStores(boost::asio::ssl::context& ctx, const std::vector<std::string>& storeNames)
{
    X509_STORE* store = X509_STORE_new();

    for (const std::string& storeName : storeNames)
    {
        HCERTSTORE hStore = CertOpenSystemStore(0, storeName.c_str());
        if (hStore == NULL)
        {
            return;
        }

        PCCERT_CONTEXT pContext = NULL;
        while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL)
        {
            // convert from DER to internal format
            X509* x509 = d2i_X509(NULL, (const unsigned char**)&pContext->pbCertEncoded, pContext->cbCertEncoded);
            if (x509 != NULL)
            {
                X509_STORE_add_cert(store, x509);
                X509_free(x509);
            }
        }

        CertFreeCertificateContext(pContext);
        CertCloseStore(hStore, 0);
    }

    // attach X509_STORE to boost ssl context
    SSL_CTX_set_cert_store(ctx.native_handle(), store);
}

void
AddWindowsRootCertificates(boost::asio::ssl::context& ctx)
{
    AddWindowsRootCertificatesFromStores(ctx, { "ROOT", "CA" });
}

template <typename StreamT>
boost::beast::http::response<boost::beast::http::dynamic_body>
HttpImpl(const Url& url, StreamT& stream, boost::beast::http::verb verb)
{
    // HTTP request
    boost::beast::http::request<boost::beast::http::string_body> request { verb, url.Target(), 11 };
    request.set(boost::beast::http::field::host, url.Host());
    request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    boost::beast::http::write(stream, request);

    // HTTP response
    boost::beast::flat_buffer buffer;
    boost::beast::http::response_parser<boost::beast::http::dynamic_body> parser;
    parser.eager(true);
    parser.body_limit(std::numeric_limits<std::uint64_t>::max());
    boost::beast::http::read(stream, buffer, parser);

    return parser.release();
}

template <typename ResultsT>
boost::beast::http::response<boost::beast::http::dynamic_body>
TcpImpl(boost::asio::io_context& context, const Url& url, const ResultsT& results, boost::beast::http::verb verb)
{
    boost::beast::tcp_stream stream(context);
    boost::beast::get_lowest_layer(stream).connect(results);
    auto response = HttpImpl(url, stream, verb);
    stream.close();
    return response;
}

template <typename ResultsT>
boost::beast::http::response<boost::beast::http::dynamic_body>
SslImpl(boost::asio::io_context& context, const Url& url, const ResultsT& results, boost::beast::http::verb verb)
{
    boost::beast::error_code ec;
    boost::asio::ssl::context ctx { boost::asio::ssl::context::tlsv12_client };

#ifdef WIN32
    AddWindowsRootCertificates(ctx);
#endif // WIN32

    ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream { context, ctx };

    if (!SSL_set_tlsext_host_name(stream.native_handle(), url.Host().c_str()))
    {
        boost::beast::error_code ec { static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
        throw boost::beast::system_error { ec };
    }

    boost::beast::get_lowest_layer(stream).connect(results);
    stream.handshake(boost::asio::ssl::stream_base::client);
    auto response = HttpImpl(url, stream, verb);
    stream.shutdown(ec);

    if (ec == boost::asio::error::eof)
        ec = {};
    if (ec == boost::asio::ssl::error::stream_truncated)
        ec = {};
    if (ec)
        throw boost::beast::system_error { ec };

    return response;
}

std::string
GetImpl(boost::asio::io_context& context, const std::string& request)
{
    const Url url = Url::Parse(request);

    // Resolve
    boost::asio::ip::tcp::resolver resolver { context };
    const auto results = resolver.resolve(url.Host(), url.Port());

    // Perform request
    const auto response = url.Ssl() ? SslImpl(context, url, results, boost::beast::http::verb::get)
                                    : TcpImpl(context, url, results, boost::beast::http::verb::get);

    // Check redirect
    const std::set<boost::beast::http::status> redirectCodes { boost::beast::http::status::moved_permanently,
                                                               boost::beast::http::status::found,
                                                               boost::beast::http::status::temporary_redirect };

    if (redirectCodes.count(response.result()) > 0)
    {
        return GetImpl(context, response[boost::beast::http::field::location].to_string());
    }

    return boost::beast::buffers_to_string(response.body().data());
}

std::string
RestClient::Get(const std::string& request)
{
    try
    {
        return GetImpl(mIoContext, Url::Encode(request));
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR << ex.what();
    }

    return "[Error]";
}

} // namespace RenderStudio::Networking
