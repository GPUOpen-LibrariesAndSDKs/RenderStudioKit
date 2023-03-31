#include "Url.h"

#pragma warning(push, 0)
#include <iomanip>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <uriparser/Uri.h>
#pragma warning(pop)

namespace RenderStudio::Networking
{

Url
Url::Parse(const std::string& request)
{
    UriUriA uri;
    const char* errorPos;

    if (uriParseSingleUriA(&uri, request.c_str(), &errorPos) != URI_SUCCESS)
    {
        throw std::runtime_error("Can't parse URL: " + request);
    }

    Url result {};

    result.mProtocol = std::string(uri.scheme.first, uri.scheme.afterLast);
    result.mHost = std::string(uri.hostText.first, uri.hostText.afterLast);
    result.mPort = std::string(uri.portText.first, uri.portText.afterLast);

    auto current = uri.pathHead;
    while (current != nullptr)
    {
        result.mTarget += "/" + std::string { current->text.first, current->text.afterLast };
        current = current->next;
    }

    result.mTarget += "?" + std::string(uri.query.first, uri.query.afterLast);

    result.mSsl = result.mProtocol == "https" || result.mProtocol == "wss";

    if (result.mPort.empty())
    {
        result.mPort = result.mProtocol == "https" ? "443" : "80";
    }

    uriFreeUriMembersA(&uri);
    return result;
}

std::string
Url::Encode(const std::string& url)
{
    std::string result = url;
    boost::replace_all(result, " ", "%20");
    return result;
}

std::ostream&
operator<<(std::ostream& os, const Url& url)
{
    os << "(" << url.Protocol() << ", " << url.Host() << ", " << url.Port() << ", " << url.Target() << ", " << url.Ssl()
       << ")\n";
    return os;
}

} // namespace RenderStudio::Networking
