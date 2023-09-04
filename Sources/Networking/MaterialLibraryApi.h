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
#include <vector>

#include <boost/json.hpp>
#pragma warning(pop)

namespace RenderStudio::Networking::MaterialLibraryAPI
{

///
/// Structures
///

struct PackageResponse
{
    struct Item
    {
        std::string id;
        std::string author;
        std::string file_url;
        float size_mb;
        std::string created_date;
        std::string updated_date;
        std::string file;
        std::string label;
    };

    std::uint32_t count;
    std::vector<Item> results;
};

///
/// Structure parsing
///

PackageResponse tag_invoke(boost::json::value_to_tag<PackageResponse>, boost::json::value const& json);
PackageResponse::Item tag_invoke(boost::json::value_to_tag<PackageResponse::Item>, boost::json::value const& json);

///
/// Network requests
///

PackageResponse GetMaterialPackage(const std::string& materialId);
std::filesystem::path Download(const PackageResponse::Item& material, const std::filesystem::path& path);

} // namespace RenderStudio::Networking::MaterialLibraryAPI
