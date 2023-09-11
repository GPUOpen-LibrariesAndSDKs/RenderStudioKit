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

#include "MaterialLibraryApi.h"

#pragma warning(push, 0)
#include <fstream>

#include <zip.h>

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#pragma warning(pop)

#include "LocalStorageApi.h"
#include "RestClient.h"

#include <Logger/Logger.h>

namespace RenderStudio::Networking::LocalStorageAPI
{

///
/// Structure parsing
///

template <class T>
void
Extract(boost::json::object const& json, T& t, boost::json::string_view key)
{
    t = boost::json::value_to<T>(json.at(key));
}

template <class T>
void
Extract(boost::json::object const& json, std::optional<T>& t, boost::json::string_view key)
{
    if (json.if_contains(key) && !json.at(key).is_null())
    {
        t = boost::json::value_to<T>(json.at(key));
    }
}

PackageResponse
tag_invoke(boost::json::value_to_tag<PackageResponse>, boost::json::value const& json)
{
    PackageResponse r;
    const boost::json::object& root = json.as_object();

    Extract(root, r.count, "count");
    Extract(root, r.results, "results");

    return r;
}

PackageResponse::Item
tag_invoke(boost::json::value_to_tag<PackageResponse::Item>, boost::json::value const& json)
{
    PackageResponse::Item r;
    const boost::json::object& root = json.as_object();

    Extract(root, r.created_at, "created_at");
    Extract(root, r.updated_at, "updated_at");
    Extract(root, r.id, "id");
    Extract(root, r.name, "name");
    Extract(root, r.type, "type");
    Extract(root, r.file, "file");
    Extract(root, r.thumbnail, "thumbnail");
    Extract(root, r.texture, "texture");

    return r;
}

///
/// Network requests
///

PackageResponse::Item
GetLightPackage(const std::string& lightId, const std::string& storageUrl)
{
    std::string response
        = RenderStudio::Networking::RestClient().Get(fmt::format("{}/api/lights/{}", storageUrl, lightId));

    boost::json::error_code ec;
    boost::json::value json = boost::json::parse(response, ec);

    return boost::json::value_to<LocalStorageAPI::PackageResponse::Item>(json);
}

std::filesystem::path
Download(const PackageResponse::Item& package, const std::filesystem::path& path, const std::string& storageUrl)
{
    auto getUsdRoot = [&]()
    {
        for (auto const& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".usda")
            {
                return entry.path();
            }
        }

        throw std::runtime_error("Can't find USDA file after download");
    };

    if (storageUrl.empty())
    {
        throw std::runtime_error("Storage URL was empty");
    }

    // Skip downloading if exists
    if (std::filesystem::exists(path))
    {
        return getUsdRoot();
    }
    else
    {
        std::filesystem::create_directories(path);
    }

    // Download light USDA file
    std::filesystem::path usdaPath = path / package.file;
    std::string usdaData
        = RenderStudio::Networking::RestClient().Get(storageUrl + "/api/lights/" + package.id + "/file");
    std::ofstream usdaOut(usdaPath, std::ios::binary);
    usdaOut << usdaData;
    usdaOut.close();

    // Download light texture
    if (package.texture.has_value())
    {
        std::filesystem::path texturePath = path / package.texture.value();
        std::string textureData
            = RenderStudio::Networking::RestClient().Get(storageUrl + "/api/lights/" + package.id + "/texture");
        std::ofstream textureOut(texturePath, std::ios::binary);
        textureOut << textureData;
        textureOut.close();
    }

    return getUsdRoot();
}

} // namespace RenderStudio::Networking::LocalStorageAPI
