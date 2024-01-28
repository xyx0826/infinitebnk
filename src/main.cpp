#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "Item.h"
#include "module/ModuleItem.h"
#include "module/ModuleManager.h"
#include "tags/TagManager.h"

#include "stderr_logger.h"

const uint32_t TAG_TYPE_SBNK = 1935830635; /* "sbnk" */

int error(const char *error) {
    std::cerr << "infinitebnk: Wwise SoundBank extractor for Halo Infinite, build 1" << std::endl;
    std::cerr << "Usage: infinitebnk.exe <path-to-deploy-folder> [path-to-tagnames]" << std::endl;
    std::cerr << "Link to download tagnames: "
        << "https://github.com/Gamergotten/Infinite-runtime-tagviewer/raw/master/files/tagnames.txt"
        << std::endl;
    std::cerr << "\nError: " << error << std::endl;

    return 1;
}

void parse_tagnames_row(
    std::map<uint32_t, std::filesystem::path> &tag_id_to_path,
    std::string &row
) {
    if (row.ends_with(".soundbank") == false) {
        /* Not for a SoundBank */
        return;
    }

    auto tag_id_be_str = row.substr(0, 8);
    auto tag_path = std::filesystem::path(row.substr(11));
    tag_path.replace_extension(".bnk");

    size_t n_conv = 0;
    uint32_t tag_id = std::stoul(tag_id_be_str, &n_conv, 16);
    tag_id = std::byteswap(tag_id);
    if (n_conv != 8) {
        /* Invalid row */
        return;
    }

    if (tag_id_to_path.count(tag_id) > 0) {
        /* Do not overwrite */
        return;
    }

    tag_id_to_path[tag_id] = tag_path;
}

void load_tagnames(
    std::filesystem::path path_to_tagnames,
    std::map<uint32_t, std::filesystem::path> &tag_id_to_path
) {
    std::ifstream tagnames(path_to_tagnames);
    if (!tagnames.is_open()) {
        std::cout << "Warning: could not load tagnames from " << path_to_tagnames << std::endl;
        return;
    }

    std::cout << "Loading tag ID to path mapping from " << path_to_tagnames << std::endl;
    std::string row;
    while (std::getline(tagnames, row)) {
        parse_tagnames_row(tag_id_to_path, row);
    }

    tagnames.close();
    std::cout << "Loaded " << tag_id_to_path.size()
        << " SoundBank tag ID to path mappings" << std::endl;
}

void load_modules(ModuleManager &module_manager, std::filesystem::path &deploy_path) {
    auto iter = std::filesystem::recursive_directory_iterator(deploy_path);
    for (const auto &entry : iter) {
        if (entry.is_regular_file() && entry.path().extension() == ".module") {
            std::cout << "Loading module: " << entry.path() << std::endl;
            auto ret = module_manager.addModule(entry.path().string().c_str());
            if (ret != 0) {
                std::cerr << "Could not load module: " << entry.path() << std::endl;
            }
        }
    }

    std::cerr << "Loaded " << module_manager.modules.size() << " modules" << std::endl;
}

void export_tag_to_csv(ModuleManager &module_manager) {
    std::cout << "tag_type,asset_id,asset_id_be,flags,module_path" << std::endl;
    for (auto const &[asset_id, item] : module_manager.assetIdItems) {
        assert(asset_id == item->assetID);

        char tag_type[5];
        auto tag_type_val = std::byteswap(item->tagType);
        memcpy(tag_type, &tag_type_val, sizeof(uint32_t));

        std::cout << std::format(
            "{},0x{},{:08x},{:#018b},\"{}\"",
            tag_type,
            item->name,
            std::byteswap(item->assetID),
            item->flags,
            item->module->path
        ) << std::endl;
    }
}

bool extract_tag(
    std::filesystem::path &extract_root,
    Tag &tag,
    std::map<uint32_t, std::filesystem::path> &tag_id_to_path
) {
    /* Extract tag data */

    auto asset_id = tag.item->moduleItem->assetID;
    auto data = reinterpret_cast<char *>(tag.item->moduleItem->extractData());
    if (data == nullptr) {
        std::cerr << "Error: failed extracting data for tag asset "
            << std::format("{:#010x}", asset_id) << std::endl;
        return false;
    }

    /* Locate SoundBank data within tag data */

    assert(tag.item->header.stringLength == 0);
    auto tag_magic_offset = data +
        tag.item->header.stringDataOffset + tag.item->header.stringLength + 16;
    assert(*reinterpret_cast<uint32_t *>(tag_magic_offset) == 0x9b555ad2);
    auto tag_unknown_count_offset = tag_magic_offset + 8;
    auto tag_unknown_count = *reinterpret_cast<uint32_t *>(tag_unknown_count_offset);
    if (tag_unknown_count == 0) {
        /* Heuristically this means SoundBank data not present */
        std::cout << "Skipped missing SoundBank from tag asset "
            << std::format("{:#010x}", asset_id) << std::endl;
        free(data);
        return false;
    }

    assert(tag_unknown_count == 0x13);
    auto soundbank_length_offset = tag_unknown_count_offset + 8 +
        tag_unknown_count * sizeof(uint32_t) + 2 + 4;
    auto soundbank_length = *reinterpret_cast<uint32_t *>(soundbank_length_offset);
    assert(soundbank_length < tag.item->moduleItem->decompressedSize);
    if (soundbank_length == 0) {
        std::cout << "Skipped empty SoundBank from tag asset "
            << std::format("{:#010x}", asset_id) << std::endl;
        free(data);
        return false;
    }

    assert(soundbank_length > 0);
    auto *soundbank_data = new char[soundbank_length];
    memcpy(soundbank_data, soundbank_length_offset + 4, soundbank_length);
    assert(*reinterpret_cast<uint32_t *>(soundbank_data) == 1145588546); /* "BKHD" */
    free(data);

    std::filesystem::path extract_path;
    if (tag_id_to_path.count(asset_id) > 0) {
        extract_path = extract_root / tag_id_to_path[asset_id];
    } else {
        auto file_name = std::format("{:#010x}.bnk", asset_id);
        extract_path = extract_root / file_name;
    }

    assert(extract_path.has_filename());
    auto extract_folder = std::filesystem::path(extract_path).remove_filename();
    std::filesystem::create_directories(extract_folder);

    auto file = std::ofstream(extract_path, std::ios::binary);
    if (file.is_open() == false) {
        std::cerr << "Error: failed creating file " << extract_path << std::endl;
        return false;
    }

    file.write(soundbank_data, soundbank_length);
    delete[] soundbank_data;
    std::cout << "Extracted " << extract_path << std::endl;
    file.close();
    return true;
}

int main(int argc, char *argv[]) {
    /* Get path to deploy folder */

    if (argc < 2) {
        return error("missing path to deploy folder.");
    }

    std::filesystem::path deploy_path(argv[1]);
    if (std::filesystem::is_directory(deploy_path) == false) {
        return error("specified path to deploy folder is invalid.");
    }

    deploy_path = std::filesystem::absolute(deploy_path);
    std::cout << "Using path to deploy folder: " << deploy_path << std::endl;

    /* Get path to tagnames */

    std::map<uint32_t, std::filesystem::path> tag_id_to_path;

    if (argc < 3 || std::filesystem::is_regular_file(argv[2]) == false) {
        std::cout << "Warning: path to tagnames is invalid or unspecified. "
            << "All extracted SoundBanks will use hexadecimal tag ID as name."
            << std::endl;
    } else {
        load_tagnames(std::filesystem::path(argv[2]), tag_id_to_path);
    }
    
    /* Create extract root folder */

    auto extract_root = std::filesystem::absolute("./soundbanks");
    std::filesystem::create_directory(extract_root);
    std::cout << "Using extraction folder: " << extract_root << std::endl;

    /* Initialize libinfinite */

    stderr_logger logger;
    ModuleManager module_manager(&logger);
    TagManager tag_manager(&module_manager, &logger);
    load_modules(module_manager, deploy_path);

    std::cout << "Building node tree..." << std::endl;
    module_manager.buildNodeTree();

    /* Dump tags */

    int file_count = 0;
    for (auto const &module : module_manager.modules) {
        std::cout << "Scanning module " << module->path << std::endl;
        for (const auto &[_, item] : module->items) {
            if (item->tagType == TAG_TYPE_SBNK) {
                tag_manager.addTag(item);
                file_count += extract_tag(
                    extract_root,
                    *tag_manager.getTag(item->assetID),
                    tag_id_to_path
                );
            }
        }
    }

    std::cout << "Successfully extracted " << file_count << " SoundBanks" << std::endl;

    /* Clean up */

    std::cout << "All done." << std::endl;
    return 0;
}
