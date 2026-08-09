// ws_server.hpp — a deliberately minimal RFC 6455 WebSocket server.
//
// Scope: local dev/test tool only. Broadcasts JSON ticks to the browser and
// decodes incoming single-frame text messages from it (e.g. "run this
// scenario"). No TLS, no frame fragmentation, no permessage-deflate. Plenty
// for a browser tab talking to a process on localhost.
//
// Linux/macOS only (POSIX sockets). Not written for Windows.
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sim {

class WsServer {
public:
    explicit WsServer(int port) : port_(port) {}

    ~WsServer();

    // Starts accepting connections on a background thread. Returns immediately.
    void start();

    void stop();

    size_t clientCount();

    // Called from a client's reader thread whenever a complete text frame
    // arrives. Set this before start(). Runs on whichever client's thread
    // received the message, not the accept thread — keep it quick or hand
    // off to your own worker if a call takes a while.
    std::function<void(const std::string&)> onMessage;

    // Sends a text frame to every currently connected client.
    void broadcast(const std::string& text);

private:
    void acceptLoop();
    bool handshake(int fd);
    // Reads and decodes incoming client frames (always masked, per spec).
    // Single-frame text messages only — plenty for a JSON scenario payload
    // from the browser; no support for fragmentation or permessage-deflate.
    void readerLoop(int fd);
    bool readExact(int fd, uint8_t* buf, uint64_t n);
    // Encodes and sends a single unmasked text frame (server->client per spec).
    // send() can legally write fewer bytes than requested — looping here is
    // required, not optional; a single short write desyncs every frame after
    // it for that client (the browser has no way to recover mid-stream).
    bool sendFrame(int fd, const std::string& payload);

    int port_;
    int listen_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread accept_thread_;
    std::vector<int> clients_;
    std::mutex clients_mutex_;
};

} // namespace sim
