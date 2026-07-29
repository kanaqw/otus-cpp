#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace parser {

template <typename T>
class SafeQueue {
    public:
        void push(T value) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(std::move(value));
            }
            cv_.notify_one();
        }

        std::optional<T> pop() {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });

            if (queue_.empty()) {
                return std::nullopt;
            }

            T value = std::move(queue_.front());
            queue_.pop();
            return value;
        }

        void stop() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                stopped_ = true;
            }
            cv_.notify_all();
        }

    private:
        std::queue<T> queue_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool stopped_ = false;
};

} // namespace parser
