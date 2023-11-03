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

#include "RestClient.h"

#pragma warning(push, 0)
#include <fstream>

#include <boost/algorithm/string.hpp>
#pragma warning(pop)

#include <Logger/Logger.h>
#include <Utils/FileUtils.h>

namespace RenderStudio::Networking::MaterialLibraryAPI
{

///
/// Network requests
///

PackageResponse
GetMaterialPackage(const std::string& materialId)
{
    std::string response = RenderStudio::Networking::RestClient().Get(
        "https://api.matlib.gpuopen.com/api/packages/?material=" + materialId);

    boost::json::error_code ec;
    boost::json::value json = boost::json::parse(response, ec);

    return boost::json::value_to<MaterialLibraryAPI::PackageResponse>(json);
}

std::filesystem::path
Download(const PackageResponse::Item& material, const std::filesystem::path& path)
{
    auto getMtlxRoot = [&]()
    {
        for (auto const& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".mtlx")
            {
                return entry.path();
            }
        }

        throw std::runtime_error("Can't find MaterialX file after download");
    };

    // Skip downloading if exists
    if (std::filesystem::exists(path))
    {
        return getMtlxRoot();
    }
    else
    {
        std::filesystem::create_directories(path);
    }

    // Download and extract archive
    RenderStudio::Utils::TempDirectory temp;
    std::filesystem::path archivePath = temp.Path() / "Package.zip";
    RenderStudio::Networking::RestClient().Download(material.file_url, archivePath);
    RenderStudio::Utils::Extract(archivePath, temp.Path() / "Package");
    RenderStudio::Utils::Move(temp.Path() / "Package", path);

    // Return file name
    return getMtlxRoot();
}

///
/// Structure parsing
///

template <class T>
void
Extract(boost::json::object const& json, T& t, boost::json::string_view key)
{
    t = boost::json::value_to<T>(json.at(key));
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

    Extract(root, r.id, "id");
    Extract(root, r.author, "author");
    Extract(root, r.file_url, "file_url");
    Extract(root, r.created_date, "created_date");
    Extract(root, r.updated_date, "updated_date");
    Extract(root, r.file, "file");
    Extract(root, r.label, "label");

    // Convert size from string "14.1 MB" to float 14.1
    std::string size_in = boost::json::value_to<std::string>(json.at("size"));
    std::vector<std::string> tokens;
    boost::split(tokens, size_in, boost::is_any_of(" "));

    if (tokens.size() != 2)
    {
        throw std::runtime_error("Wrong tokens count in deserializing size");
    }

    float size = std::stof(tokens.at(0));

    if (tokens.at(1) == "MB")
    {
        size *= 1;
    }
    else if (tokens.at(1) == "KB")
    {
        size /= 1024;
    }
    else if (tokens.at(1) == "GB")
    {
        size *= 1024;
    }
    else
    {
        throw std::runtime_error("Wrong size token in deserializing size");
    }

    r.size_mb = size;

    return r;
}

} // namespace RenderStudio::Networking::MaterialLibraryAPI
