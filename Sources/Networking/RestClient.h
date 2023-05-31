#pragma once

#pragma warning(push, 0)
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

private:
    boost::asio::io_context mIoContext;
};

} // namespace RenderStudio::Networking