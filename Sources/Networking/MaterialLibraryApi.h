#pragma once

#include <vector>
#include <filesystem>
#include <boost/json.hpp>

namespace RenderStudio::Networking::MaterialLibraryAPI {

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

}
