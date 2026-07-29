#include <iostream>

#include "async.h"

int main(int argc, char* argv[]) {

    std::size_t bulk_size = 3;
    if (argc > 1) {
        try {
            int parsed = std::stoi(argv[1]);
            if (parsed < 0) {
                throw std::out_of_range("Negative value not allowed");
            }
            bulk_size = static_cast<std::size_t>(parsed);
        } catch (...) {
            std::cerr << "Invalid argument, using default size - 3\n";
        }
    }

    async::handle_t handle = async::connect(bulk_size);

    char buffer[64];
    while (std::cin.read(buffer, sizeof(buffer)) || std::cin.gcount() > 0) {
        async::receive(handle, buffer, static_cast<std::size_t>(std::cin.gcount()));
    }

    async::disconnect(handle);

    return 0;
}
