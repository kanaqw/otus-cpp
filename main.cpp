#include <iostream>
#include "include/parser.hpp"

int main(int argc, char* argv[]) {
    size_t default_size = 3;
    if (argc > 1){
        try {
            long long parsed = std::stoul(argv[1]);
            if (parsed < 0) {
                throw std::out_of_range("Negative value not allowed");
            }
            default_size = static_cast<size_t>(parsed);
        } catch (...) {
            std::cerr << "Invalid argument, using default size - 3\n";
        }
    }

    parser::PackHandler handler(default_size);
    auto console_logger = std::make_shared<parser::ConsoleLogger>();
    auto file_logger = std::make_shared<parser::FileLogger>();

    handler.subscribe(console_logger);
    handler.subscribe(file_logger);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        handler.add_cmd_to_pack(line);
    }

    handler.flush_eof();

    return 0;
}
