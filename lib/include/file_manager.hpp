#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <map>
#include <set>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/uuid/detail/sha1.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

enum class HashType { CRC32, MD5, SHA1 };

struct Config {
    std::vector<std::string> include_paths;
    std::vector<std::string> exclude_paths;
    int level = 0; 
    uintmax_t min_file_size = 1;
    std::vector<std::string> file_masks;
    size_t block_size = 1024;
    HashType hash_type = HashType::CRC32;
};

std::string calculate_hash(const std::vector<char>& block, HashType type);
bool match_masks(const std::string& filename, const std::vector<std::regex>& regexes);
std::regex mask_to_regex(const std::string& mask);
void scan_directory(const fs::path& current_path, int current_level, const Config& config,
                    const std::vector<std::regex>& regex_masks,
                    std::map<uintmax_t, std::vector<fs::path>>& files_by_size);

class FileSignatureManager{
public:
    FileSignatureManager(fs::path path, size_t block_size, HashType hash_type)
        : path_(std::move(path)), block_size_(block_size), hash_type_(hash_type) {}
    std::string get_block_hash(size_t block_idx);
    const fs::path& get_path() const { return path_; };

private:
    fs::path path_;
    size_t block_size_;
    HashType hash_type_;
    std::vector<std::string> cached_hashes_;
    bool is_eof_ = false;
};
