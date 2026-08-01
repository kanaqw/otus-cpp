#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <memory>
#include <stack>

#include <atomic>
#include <mutex>
#include <thread>

#include "safe_queue.hpp"

enum class CmdType : size_t {
    STATIC = 0,
    DYNAMIC
};

namespace parser {

    struct Bulk {
        std::vector<std::string> commands;
        time_t time;
    };

    class IOListener {
        public:
        virtual void update(const std::vector<std::string>& block,
                            time_t time) = 0;
        virtual ~IOListener() = default;
    };

    class ConsoleLogger : public IOListener {
        public:
        ConsoleLogger();
        ~ConsoleLogger() override;

        void update(const std::vector<std::string>& block,
                            time_t time) override;

        private:
            void worker_loop();

            SafeQueue<Bulk> queue_;
            std::thread thread_;
    };

    class FileLogger : public IOListener {
        public:
        FileLogger();
        ~FileLogger() override;

        void update(const std::vector<std::string>& block,
                            time_t time) override;

        private:
            void worker_loop();

            SafeQueue<Bulk> queue_;
            std::atomic<size_t> file_seq_{0};
            std::thread file_1_;
            std::thread file_2_;
    };

    // Process-wide singletons: shared across every connect() context so only
    // three background threads ever exist, not three per context.
    ConsoleLogger& console_logger();
    FileLogger& file_logger();

    // Shared across every connection using the same bulk size
    class StaticBulkAggregator {
        public:
        explicit StaticBulkAggregator(size_t N) : packSize_(N) {}

        void subscribe(IOListener* listener) {
            listeners_.push_back(listener);
        }

        void register_session();
        void unregister_session();
        void add(const std::string& cmd);

        private:
            void notify();

            std::mutex mutex_;
            std::vector<std::string> commands_;
            std::vector<IOListener*> listeners_;
            size_t packSize_;
            time_t timestamp_{};
            size_t active_sessions_{0};
    };

    StaticBulkAggregator& static_aggregator(size_t bulk);

    class PackHandler {
        public:
        // Self-contained mode: static blocks accumulate and flush locally.
        // Used by tests and by the single-connection CLI.
        PackHandler(size_t N)
        : currentState_(CmdType::STATIC),
          packSize_(N),
          timestamp_(0),
          brackets_()
          {}

        // Delegated mode: static commands are forwarded to a shared
        // aggregator so they can mix with other connections' static
        // commands. Dynamic (bracketed) blocks stay local, as before.
        PackHandler(size_t N, StaticBulkAggregator& aggregator)
        : currentState_(CmdType::STATIC),
          packSize_(N),
          timestamp_(0),
          brackets_(),
          aggregator_(&aggregator)
          {}

        inline void subscribe(IOListener* listener) {
            listeners_.push_back(listener);
        }
        void add_cmd_to_pack(const std::string& cmd);
        void flush_eof();

        private:
            void handle_open_bracket();
            void handle_close_bracket();
            void handle_regular_cmd(const std::string& cmd);
            void notify();

            std::vector<std::string> commands_;
            std::vector<IOListener*> listeners_;
            CmdType currentState_{};
            size_t packSize_;
            time_t timestamp_;
            std::stack<char> brackets_;
            StaticBulkAggregator* aggregator_{nullptr};
    };

} //namespace parser



