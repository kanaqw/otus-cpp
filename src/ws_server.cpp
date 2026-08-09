#include "ws_server.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sha1.hpp"

namespace sim {

WsServer::~WsServer() { stop(); }

void WsServer::start() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("ws_server: bind failed on port " + std::to_string(port_));
    }
    listen(listen_fd_, 8);
    running_ = true;
    accept_thread_ = std::thread([this] { acceptLoop(); });
    std::cout << "[ws_server] listening on ws://localhost:" << port_ << "\n";
}

void WsServer::stop() {
    if (!running_) return;
    running_ = false;
    if (listen_fd_ >= 0) { ::shutdown(listen_fd_, SHUT_RDWR); ::close(listen_fd_); listen_fd_ = -1; }
    if (accept_thread_.joinable()) accept_thread_.join();
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (int fd : clients_) ::close(fd);
    clients_.clear();
}

size_t WsServer::clientCount() {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

void WsServer::broadcast(const std::string& text) {
    std::vector<int> dead;
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (int fd : clients_) {
        if (!sendFrame(fd, text)) dead.push_back(fd);
    }
    for (int fd : dead) {
        ::close(fd);
        clients_.erase(std::remove(clients_.begin(), clients_.end(), fd), clients_.end());
    }
}

void WsServer::acceptLoop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int fd = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (fd < 0) { if (running_) continue; else break; }
        if (handshake(fd)) {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_.push_back(fd);
            std::cout << "[ws_server] client connected (" << clients_.size() << " total)\n";
            // Detach a reader thread just to notice disconnects / ignore incoming frames.
            std::thread([this, fd] { readerLoop(fd); }).detach();
        } else {
            ::close(fd);
        }
    }
}

bool WsServer::handshake(int fd) {
    char buf[4096];
    ssize_t n = ::recv(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return false;
    buf[n] = '\0';
    std::string req(buf);

    auto pos = req.find("Sec-WebSocket-Key:");
    if (pos == std::string::npos) return false;
    pos += std::strlen("Sec-WebSocket-Key:");
    auto end = req.find("\r\n", pos);
    std::string key = req.substr(pos, end - pos);
    // trim
    while (!key.empty() && (key.front() == ' ')) key.erase(key.begin());
    while (!key.empty() && (key.back() == ' ' || key.back() == '\r')) key.pop_back();

    static const std::string guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    auto digest = sha1(key + guid);
    std::string accept = base64_encode(digest.data(), digest.size());

    std::string resp =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    return ::send(fd, resp.data(), resp.size(), 0) > 0;
}

void WsServer::readerLoop(int fd) {
    while (running_) {
        uint8_t header[2];
        if (!readExact(fd, header, 2)) break;

        uint8_t opcode = header[0] & 0x0F;
        bool masked = header[1] & 0x80;
        uint64_t len = header[1] & 0x7F;

        if (len == 126) {
            uint8_t ext[2];
            if (!readExact(fd, ext, 2)) break;
            len = (static_cast<uint16_t>(ext[0]) << 8) | ext[1];
        } else if (len == 127) {
            uint8_t ext[8];
            if (!readExact(fd, ext, 8)) break;
            len = 0;
            for (int i = 0; i < 8; ++i) len = (len << 8) | ext[i];
        }

        uint8_t maskKey[4] = {0, 0, 0, 0};
        if (masked && !readExact(fd, maskKey, 4)) break;

        std::string payload(len, '\0');
        if (len > 0 && !readExact(fd, reinterpret_cast<uint8_t*>(&payload[0]), len)) break;
        if (masked) for (uint64_t i = 0; i < len; ++i) payload[i] = payload[i] ^ maskKey[i % 4];

        if (opcode == 0x8) break;               // close
        if (opcode == 0x1 && onMessage) onMessage(payload); // text
        // ping(0x9)/pong(0xA)/binary(0x2): ignored, fine for this tool
    }
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(std::remove(clients_.begin(), clients_.end(), fd), clients_.end());
    ::close(fd);
}

bool WsServer::readExact(int fd, uint8_t* buf, uint64_t n) {
    uint64_t got = 0;
    while (got < n) {
        ssize_t r = ::recv(fd, buf + got, n - got, 0);
        if (r <= 0) return false;
        got += static_cast<uint64_t>(r);
    }
    return true;
}

bool WsServer::sendFrame(int fd, const std::string& payload) {
    std::string frame;
    frame.push_back(static_cast<char>(0x81)); // FIN + text opcode

    size_t len = payload.size();
    if (len <= 125) {
        frame.push_back(static_cast<char>(len));
    } else if (len <= 65535) {
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    } else {
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i)
            frame.push_back(static_cast<char>((len >> (i * 8)) & 0xFF));
    }
    frame += payload;

    size_t sent = 0;
    while (sent < frame.size()) {
        ssize_t n = ::send(fd, frame.data() + sent, frame.size() - sent, MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

} // namespace sim
