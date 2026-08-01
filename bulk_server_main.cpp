#include <iostream>
#include <stdexcept>

#include <boost/asio.hpp>

#include "server.hpp"

namespace {

struct Args {
    unsigned short port;
    std::size_t bulk_size;
};

Args parse_args(int argc, char* argv[]) {
    if (argc != 3) {
        throw std::runtime_error("usage: bulk_server <port> <bulk_size>");
    }

    int port_parsed = std::stoi(argv[1]);
    int size_parsed = std::stoi(argv[2]);
    if (port_parsed < 0 || port_parsed > 65535 || size_parsed < 0) {
        throw std::out_of_range("port/bulk_size out of range");
    }

    return Args{static_cast<unsigned short>(port_parsed),
                static_cast<std::size_t>(size_parsed)};
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        Args args = parse_args(argc, argv);

        boost::asio::io_context io_context;
        bulk_server::Server server(io_context, args.port, args.bulk_size);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "bulk_server error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
