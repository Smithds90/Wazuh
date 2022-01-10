#include <filesystem>
#include <fstream>
#include <iostream>

#include "diskStorage.hpp"
#include "rapidjson/istreamwrapper.h"

constexpr std::string_view ext_json_schema {".json"};
constexpr std::string_view ext_other_asset {".yml"};

// Overridden method
std::vector<std::string> diskStorage::getAssetList(const AssetType type)
{
    using std::string;

    std::vector<string> assetList {};
    std::filesystem::path base_dir {this->path};

    // Get the path to the asset directory
    base_dir /= assetTypeToPath.at(type);

    for (const auto& entry : std::filesystem::directory_iterator(base_dir))
    {
        if (entry.is_regular_file() && entry.path().has_extension())
        {
            // Only the json schema has the json extension
            if (entry.path().extension().string() == ext_other_asset ||
                    (entry.path().extension().string() == ext_json_schema && type == AssetType::Schemas))
            {
                assetList.push_back(string {entry.path().stem().string()});
                //std::cout << "Adding file: '" << entry.path() << std::endl;
            }
        }

        // else {
        //     std::cout << "ignoring file: '" << entry.path() << std::endl;
        // }
    }

    return assetList;
}

// Overridden method
std::string diskStorage::getAsset(const AssetType type, std::string_view assetName)
{
    using std::string;
    using std::string_view;
    using rapidjson::Document;
    namespace fs = std::filesystem;

    fs::path base_dir {this->path};
    string assetStr {};

    /* Get the path to the asset directory */
    base_dir /= assetTypeToPath.at(type);

    /* Get the file name to the asset file */
    string file_name {assetName};

    if (type == AssetType::Schemas)
    {
        file_name.append(ext_json_schema);
    }
    else
    {
        file_name.append(ext_other_asset);
    }

    /* Get full path to the asset file */
    fs::path file_path {base_dir / file_name};

    // Throws std::filesystem::filesystem_error
    if (fs::exists(file_path))
    {
        // Throw a
        std::ifstream ifs {file_path.string()};

        if (ifs)
        {
            std::ostringstream oss {};
            oss << ifs.rdbuf();
            assetStr = oss.str();
        }

        // else {
        //     std::cerr << "Error reading file: " << file_path << std::endl;
        // }
    }

    // else
    // {
    //     std::cout << "Asset file not found: " << file_path << std::endl;
    // }

    return assetStr;
}
