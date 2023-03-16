#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>

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


}