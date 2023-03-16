#include "MaterialLibraryApi.h"

#include <fstream>
#include <boost/algorithm/string.hpp>
#include "RestClient.h"
#include <Logger/Logger.h>
#include <zip.h>

namespace RenderStudio::Networking::MaterialLibraryAPI
{

///
/// Network requests
///

PackageResponse
GetMaterialPackage(const std::string& materialId)
{
    std::string response = RenderStudio::Networking::RestClient().Get("https://api.matlib.gpuopen.com/api/packages/?material=" + materialId);

    boost::json::error_code ec;
    boost::json::value json = boost::json::parse(response, ec);

    return boost::json::value_to<MaterialLibraryAPI::PackageResponse>(json);
}

std::filesystem::path
Download(const PackageResponse::Item& material, const std::filesystem::path& path)
{
    auto getMtlxRoot = [&]() {
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

    // Download archive
    std::filesystem::path archivePath = path / "Package.zip";
    std::string data = RenderStudio::Networking::RestClient().Get(material.file_url);
    std::ofstream out(archivePath, std::ios::binary);
    out << data;
    out.close();

    // Extract archive
    int arg = 0;
    auto status = zip_extract(
        archivePath.string().c_str(),
        path.string().c_str(),
        [](auto a, auto b)
        {
            (void)a, void(b);
            return 0;
        },
        &arg);

    // Remove archive
    std::filesystem::remove(archivePath);

    // Return file name
    return getMtlxRoot();
}

///
/// Structure parsing
///

template<class T>
void Extract(boost::json::object const& json, T& t, boost::json::string_view key)
{
    t = boost::json::value_to<T>(json.at(key));
}

PackageResponse tag_invoke(boost::json::value_to_tag<PackageResponse>, boost::json::value const& json)
{
    PackageResponse r;
    const boost::json::object& root = json.as_object();

    Extract(root, r.count, "count");
    Extract(root, r.results, "results");

    return r;
}

PackageResponse::Item tag_invoke(boost::json::value_to_tag<PackageResponse::Item>, boost::json::value const& json)
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

    if (tokens.size() != 2) {
        throw std::runtime_error("Wrong tokens count in deserializing size");
    }

    float size = std::stof(tokens.at(0));

    if (tokens.at(1) == "MB") {
        size *= 1;
    } else if (tokens.at(1) == "KB") {
        size /= 1024;
    } else if (tokens.at(1) == "GB") {
        size *= 1024;
    } else {
        throw std::runtime_error("Wrong size token in deserializing size");
    }

    r.size_mb = size;

    return r;
}

} // namespace RenderStudio::Networking::MaterialLibraryAPI
