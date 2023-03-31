#pragma once

#pragma warning(push, 0)
#include <vector>
#include <filesystem>
#include <boost/json.hpp>
#include <optional>
#pragma warning(pop)

namespace RenderStudio::Networking::LocalStorageAPI {

///
/// Structures
///

struct PackageResponse
{
    struct Item
    {
        std::string created_at;
        std::string updated_at;
        std::string id;
        std::string name;
        std::string type;
        std::string file;
        std::string thumbnail;
        std::optional<std::string> texture;
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

PackageResponse::Item GetLightPackage(const std::string& lightId, const std::string& storageUrl);
std::filesystem::path Download(const PackageResponse::Item& package, const std::filesystem::path& path, const std::string& storageUrl);

}
