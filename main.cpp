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


std::string calculate_hash(const std::vector<char>& block, HashType type) {
    if (type == HashType::CRC32) {
        boost::crc_32_type result;
        result.process_bytes(block.data(), block.size());
        return std::to_string(result.checksum());
    } 
    else if (type == HashType::MD5) {
        boost::uuids::detail::md5 hash;
        boost::uuids::detail::md5::digest_type digest;
        hash.process_bytes(block.data(), block.size());
        hash.get_digest(digest);
        const auto* int_digest = reinterpret_cast<const unsigned int*>(digest);
        std::string result;
        for (int i = 0; i < 4; ++i) {
            result += std::to_string(int_digest[i]);
        }
        return result;
    } 
    else { // SHA1
        boost::uuids::detail::sha1 hash;
        boost::uuids::detail::sha1::digest_type digest;
        hash.process_bytes(block.data(), block.size());
        hash.get_digest(digest);
        const auto* int_digest = reinterpret_cast<const unsigned int*>(digest);
        std::string result;
        for (int i = 0; i < 5; ++i) {
            result += std::to_string(int_digest[i]);
        }
        return result;
    }
}

bool match_masks(const std::string& filename, const std::vector<std::regex>& regexes) {
    if (regexes.empty()) return true;
    for (const auto& r : regexes) {
        if (std::regex_match(filename, r)) return true;
    }
    return false;
}


std::regex mask_to_regex(const std::string& mask) {
    std::string r = std::regex_replace(mask, std::regex(R"(\.)"), R"(\.)");
    r = std::regex_replace(r, std::regex(R"(\*)"), R"(.*)");
    r = std::regex_replace(r, std::regex(R"(\?)"), R"(.)");
    return std::regex(r, std::regex_constants::icase);
}

void scan_directory(const fs::path& current_path, int current_level, const Config& config,
                    const std::vector<std::regex>& regex_masks,
                    std::map<uintmax_t, std::vector<fs::path>>& files_by_size) {
    
   
    for (const auto& ex : config.exclude_paths) {
        if (fs::equivalent(current_path, fs::path(ex))) return;
    }

    if (!fs::exists(current_path) || !fs::is_directory(current_path)) return;

    for (const auto& entry : fs::directory_iterator(current_path)) {
        if (fs::is_directory(entry.status())) {
            if (current_level < config.level) {
                scan_directory(entry.path(), current_level + 1, config, regex_masks, files_by_size);
            }
        } 
        else if (fs::is_regular_file(entry.status())) {
            uintmax_t size = fs::file_size(entry.path());
        
            if (size < config.min_file_size) continue;

            if (!match_masks(entry.path().filename().string(), regex_masks)) continue;

            files_by_size[size].push_back(entry.path());
        }
    }
}

class FileSignatureManager {
public:
    FileSignatureManager(fs::path path, size_t block_size, HashType hash_type)
        : path_(std::move(path)), block_size_(block_size), hash_type_(hash_type), stream_(path_, std::ios::binary) {}

    std::string get_block_hash(size_t block_idx) {
        if (block_idx < cached_hashes_.size()) {
            return cached_hashes_[block_idx];
        }

        if (!stream_ || is_eof_) {
            return "";
        }

        std::vector<char> buffer(block_size_);
        stream_.seekg(block_idx * block_size_, std::ios::beg);
        stream_.read(buffer.data(), block_size_);
        
        size_t read_bytes = stream_.gcount();
        if (read_bytes == 0) {
            is_eof_ = true;
            return "";
        }
        buffer.resize(read_bytes);

        if (read_bytes < block_size_) {
            buffer.insert(buffer.end(), block_size_ - read_bytes, 0);
            is_eof_ = true;
        }

        std::string hash = calculate_hash(buffer, hash_type_);
        cached_hashes_.push_back(hash);
        return hash;
    }

    const fs::path& get_path() const { return path_; }

private:
    fs::path path_;
    size_t block_size_;
    HashType hash_type_;
    std::ifstream stream_;
    std::vector<std::string> cached_hashes_;
    bool is_eof_ = false;
};

int main(int argc, char* argv[]) {
    try {
        Config config;
        std::string hash_str;

        po::options_description desc("Duplicate file search tool options");
        desc.add_options()
            ("help,h", "Show help")
            ("path,p", po::value<std::vector<std::string>>(&config.include_paths)->multitoken(), "Directories to scan")
            ("exclude,e", po::value<std::vector<std::string>>(&config.exclude_paths)->multitoken(), "Directories to exclude from scan")
            ("level,l", po::value<int>(&config.level)->default_value(0), "Depth of scan (0 - only current dir)")
            ("min_size,s", po::value<uintmax_t>(&config.min_file_size)->default_value(1), "Minimal file size in byte")
            ("mask,m", po::value<std::vector<std::string>>(&config.file_masks)->multitoken(), "File name mask")
            ("block_size,b", po::value<size_t>(&config.block_size)->default_value(1024), "Block size to read")
            ("hash,f", po::value<std::string>(&hash_str)->default_value("crc32"), "hash algo (crc32, md5, sha1)");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        if (config.include_paths.empty()) {
            std::cerr << "Error: Put at least one directory to scan (--path).\n";
            return 1;
        }

        if (hash_str == "md5") config.hash_type = HashType::MD5;
        else if (hash_str == "sha1") config.hash_type = HashType::SHA1;
        else config.hash_type = HashType::CRC32;

        std::vector<std::regex> regex_masks;
        for (const auto& mask : config.file_masks) {
            regex_masks.push_back(mask_to_regex(mask));
        }

        std::map<uintmax_t, std::vector<fs::path>> files_by_size;
        for (const auto& path : config.include_paths) {
            scan_directory(fs::path(path), 0, config, regex_masks, files_by_size);
        }

        for (auto& [size, paths] : files_by_size) {
            if (paths.size() < 2) continue; 

            std::vector<std::shared_ptr<FileSignatureManager>> managers;
            for (const auto& p : paths) {
                managers.push_back(std::make_shared<FileSignatureManager>(p, config.block_size, config.hash_type));
            }

            std::map<std::vector<std::string>, std::vector<std::shared_ptr<FileSignatureManager>>> duplicate_groups;
            duplicate_groups[{}] = managers; 

            size_t block_idx = 0;
            bool process = true;

            while (process) {
                process = false;
                std::map<std::vector<std::string>, std::vector<std::shared_ptr<FileSignatureManager>>> next_groups;

                for (auto& [history, file_list] : duplicate_groups) {
                    if (file_list.size() < 2) {
                        continue;
                    }

                    std::map<std::string, std::vector<std::shared_ptr<FileSignatureManager>>> split_bucket;
                    bool has_more_blocks = false;

                    for (auto& file : file_list) {
                        std::string h = file->get_block_hash(block_idx);
                        split_bucket[h].push_back(file);
                        if (!h.empty()) {
                            has_more_blocks = true;
                        }
                    }

                    for (auto& [current_hash, split_files] : split_bucket) {
                        if (current_hash.empty()) {
                            if (split_files.size() > 1) {
                                std::setstd::string output_group;
                                for (const auto& f : split_files) {
                                    output_group.insert(f->get_path().string());
                                }
                                for (const auto& path_str : output_group) {
                                    std::cout << path_str << "\n";
                                }
                                std::cout << "\n"; 
                            }
                        }
                        else {
                            auto next_history = history;
                            next_history.push_back(current_hash);
                            next_groups[next_history] = split_files;
                            if (split_files.size() > 1 && has_more_blocks) {
                                process = true;
                            }
                        }
                    }
                }
                duplicate_groups = std::move(next_groups);
                block_idx++;}
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }
