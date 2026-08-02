#include "file_manager.hpp"

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

    boost::system::error_code ec;
    fs::directory_iterator it(current_path, ec);
    if (ec) {
        std::cerr << "Warning: cannot read directory " << current_path << ": " << ec.message() << "\n";
        return;
    }
    const fs::directory_iterator end;

    for (; it != end; it.increment(ec)) {
        if (ec) {
            std::cerr << "Warning: error while scanning directory " << current_path << ": " << ec.message() << "\n";
            break;
        }

        fs::file_status status = it->status(ec);
        if (ec) {
            std::cerr << "Warning: cannot access " << it->path() << ": " << ec.message() << "\n";
            continue;
        }

        if (fs::is_directory(status)) {
            if (current_level < config.level) {
                scan_directory(it->path(), current_level + 1, config, regex_masks, files_by_size);
            }
        }
        else if (fs::is_regular_file(status)) {
            uintmax_t size = fs::file_size(it->path(), ec);
            if (ec) {
                std::cerr << "Warning: cannot get size of " << it->path() << ": " << ec.message() << "\n";
                continue;
            }

            if (size < config.min_file_size) continue;

            if (!match_masks(it->path().filename().string(), regex_masks)) continue;

            files_by_size[size].push_back(it->path());
        }
    }
}
std::string FileSignatureManager::get_block_hash(size_t block_idx) {
    if (block_idx < cached_hashes_.size()) {
        return cached_hashes_[block_idx];
    }

    if (is_eof_) {
        return "";
    }

    // Open the file only for the duration of this read instead of holding a
    // descriptor for the manager's whole lifetime
    std::ifstream stream(path_.string(), std::ios::binary);
    if (!stream) {
        is_eof_ = true;
        return "";
    }

    std::vector<char> buffer(block_size_);
    stream.seekg(block_idx * block_size_, std::ios::beg);
    stream.read(buffer.data(), block_size_);

    size_t read_bytes = stream.gcount();
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

