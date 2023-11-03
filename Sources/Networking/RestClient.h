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

#pragma once

#pragma warning(push, 0)
#include <filesystem>
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
    std::string Put(const std::string& request, const std::string& body);
    void Download(const std::string& request, const std::filesystem::path& location);

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
