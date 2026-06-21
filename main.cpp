#include <iostream>
#include "parser.hpp"

int main(int argc, char* argv[]) {
    size_t default_size = 3;
    if (argc > 1){
        try {
            default_size = std::stoul(argv[1]);
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
