#include "include/file_manager.hpp"

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
