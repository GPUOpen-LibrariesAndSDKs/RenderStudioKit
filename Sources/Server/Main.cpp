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

#include <iostream>

#pragma warning(push, 0)
#include <list>
#include <numeric>

#include <boost/asio/ip/address.hpp>
#include <boost/json/src.hpp>
#pragma warning(pop)

#include "Logic.h"

#include <Logger/Logger.h>
#include <Networking/WebsocketServer.h>

auto
main() -> int
try
{
    const boost::asio::ip::address address = boost::asio::ip::make_address("0.0.0.0");
    const std::uint16_t port = 10000;
    const std::int32_t threads = 10;

    LOG_INFO << "Server started at " << address << ":" << port << " (threads: " << threads << ")";

    boost::asio::io_context ioc { threads };
    Logic logic;

    auto server = RenderStudio::Networking::WebsocketServer::Create(
        ioc, boost::asio::ip::tcp::endpoint { address, port }, logic);

    server->Run();

    std::vector<std::thread> workers;
    workers.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
    {
        workers.emplace_back([&ioc]() { ioc.run(); });
    }
    ioc.run();

    return EXIT_SUCCESS;
}
catch (const std::exception& ex)
{
    LOG_FATAL << "[Server Fatal Exception] " << ex.what();
    return EXIT_FAILURE;
}
