#include "server.hpp"

namespace bulk_server {

namespace asio = boost::asio;
using asio::ip::tcp;

TcpSession::TcpSession(tcp::socket socket, std::size_t bulk_size)
    : socket_(std::move(socket)),
      handle_(async::connect(bulk_size)) {}

void TcpSession::start() {
    do_read();
}

void TcpSession::do_read() {
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t length) {
            if (length > 0) {
                async::receive(handle_, buffer_.data(), length);
            }

            if (!ec) {
                do_read();
            } else {
                async::disconnect(handle_);
            }
        });
}

Server::Server(asio::io_context& io_context, unsigned short port, std::size_t bulk_size)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      bulk_size_(bulk_size) {
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<TcpSession>(std::move(socket), bulk_size_)->start();
            }
            do_accept();
        });
}

} // namespace bulk_server
