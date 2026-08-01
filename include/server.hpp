#pragma once

#include <array>
#include <cstddef>
#include <memory>

#include <boost/asio.hpp>

#include "async.h"

namespace bulk_server {

class TcpSession : public std::enable_shared_from_this<TcpSession> {
    public:
        TcpSession(boost::asio::ip::tcp::socket socket, std::size_t bulk_size);

        void start();

    private:
        void do_read();

        boost::asio::ip::tcp::socket socket_;
        std::array<char, 4096> buffer_;
        async::handle_t handle_;
};

class Server {
    public:
        Server(boost::asio::io_context& io_context, unsigned short port, std::size_t bulk_size);

    private:
        void do_accept();

        boost::asio::ip::tcp::acceptor acceptor_;
        std::size_t bulk_size_;
};

} // namespace bulk_server
