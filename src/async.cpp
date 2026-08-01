#include "async.h"
#include "parser.hpp"

#include <mutex>
#include <string>

namespace async {

namespace {

class Session {
    public:
        explicit Session(std::size_t bulk_size)
            : handler_(bulk_size) {
            handler_.subscribe(&parser::console_logger());
            handler_.subscribe(&parser::file_logger());
        }

        void receive(const char* data, std::size_t size) {
            std::lock_guard<std::mutex> lock(mutex_);

            buffer_.append(data, size);

            std::size_t start = 0;
            std::size_t pos;
            while ((pos = buffer_.find('\n', start)) != std::string::npos) {
                std::string line = buffer_.substr(start, pos - start);
                if (!line.empty()) {
                    handler_.add_cmd_to_pack(line);
                }
                start = pos + 1;
            }
            buffer_.erase(0, start);
        }

        void disconnect() {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!buffer_.empty()) {
                handler_.add_cmd_to_pack(buffer_);
                buffer_.clear();
            }
            handler_.flush_eof();
        }

    private:
        parser::PackHandler handler_;
        std::string buffer_;
        std::mutex mutex_;
};

} // namespace

handle_t connect(std::size_t bulk) {
    return new Session(bulk);
}

void receive(handle_t handle, const char* data, std::size_t size) {
    if (handle == nullptr || data == nullptr || size == 0) {
        return;
    }
    static_cast<Session*>(handle)->receive(data, size);
}

void disconnect(handle_t handle) {
    if (handle == nullptr) {
        return;
    }
    auto* session = static_cast<Session*>(handle);
    session->disconnect();
    delete session;
}

} // namespace async
