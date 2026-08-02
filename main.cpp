#include <cstdlib>
#include <iostream>

#include "database.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: join_server <port>\n";
        return 1;
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "invalid port: " << argv[1] << "\n";
        return 1;
    }

    runServer(port);
    return 0;
}
