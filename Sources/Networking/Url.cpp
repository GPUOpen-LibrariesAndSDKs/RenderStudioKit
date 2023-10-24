// Copyright 2023 Advanced Micro Devices, Inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Url.h"

#pragma warning(push, 0)
#include <iomanip>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <uriparser/Uri.h>
#pragma warning(pop)

#include <Logger/Logger.h>

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

    // Parse basic fields
    result.mProtocol = std::string(uri.scheme.first, uri.scheme.afterLast);
    result.mHost = std::string(uri.hostText.first, uri.hostText.afterLast);
    result.mPort = std::string(uri.portText.first, uri.portText.afterLast);

    // Parse full path
    auto current = uri.pathHead;
    while (current != nullptr)
    {
        result.mPath += "/" + std::string { current->text.first, current->text.afterLast };
        current = current->next;
    }

    // Parse full query
    std::string query = std::string(uri.query.first, uri.query.afterLast);
    std::string key, value;
    std::istringstream ss(query);
    while (std::getline(ss, key, '=') && std::getline(ss, value, '&'))
    {
        result.mQuery[key] = RenderStudio::Networking::Url::Decode(value);
    }

    // Parse target = path + query
    result.mTarget = result.mPath;

    if (!query.empty())
    {
        result.mTarget += "?" + query;
    }

    // Set port from protocol
    if (result.mPort.empty())
    {
        if (result.mProtocol == "https" || result.mProtocol == "wss" || result.mProtocol == "gpuopen")
        {
            result.mPort = "443";
        }
        else if (result.mProtocol == "http" || result.mProtocol == "ws")
        {
            result.mPort = "80";
        }
        else
        {
            throw std::runtime_error("Unsupported protocol: " + result.mProtocol);
        }
    }

    // Determine SSL
    result.mSsl = result.mPort == "443";

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

std::string
Url::Decode(const std::string& url)
{
    std::string result = url;
    boost::replace_all(result, "%20", " ");
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
