#pragma once

#pragma warning(push, 0)
#include <map>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#pragma warning(pop)

#include "Url.h"

namespace RenderStudio::Networking
{

class RestClient
{
public:
    std::string Get(const std::string& request);
    std::string Post(const std::string& request, const std::string& body);

    enum class Parameters
    {
        ContentType,
        Authorization
    };

    RestClient() = default;
    RestClient(const std::map<Parameters, std::string>& parameters);

private:
    boost::asio::io_context mIoContext;
    std::map<Parameters, std::string> mParameters;
};

} // namespace RenderStudio::Networking