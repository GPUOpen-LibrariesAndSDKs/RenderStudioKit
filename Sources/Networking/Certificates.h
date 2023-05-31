#pragma once

#pragma warning(push, 0)
#include <string>
#include <vector>

#include <boost/asio/ssl/context.hpp>
#pragma warning(pop)

namespace RenderStudio::Networking
{

void AddWindowsRootCertificatesFromStores(boost::asio::ssl::context& ctx, const std::vector<std::string>& storeNames);
void AddWindowsRootCertificates(boost::asio::ssl::context& ctx);

} // namespace RenderStudio::Networking
