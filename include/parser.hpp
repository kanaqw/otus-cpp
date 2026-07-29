#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <memory>
#include <stack>

#include <atomic>
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
            void worker_loop(SafeQueue<Bulk>& queue);

            SafeQueue<Bulk> queue1_;
            SafeQueue<Bulk> queue2_;
            std::atomic<size_t> counter_{0};
            std::thread file_1_;
            std::thread file_2_;
    };

    class PackHandler {
        public:
        PackHandler(size_t N) 
        : currentState_(CmdType::STATIC),
          packSize_(N),
          timestamp_(0),
          brackets_()
          {}

        inline void subscribe(std::shared_ptr<IOListener> listener) {
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
            std::vector<std::shared_ptr<IOListener>> listeners_;
            CmdType currentState_{};
            size_t packSize_;
            time_t timestamp_;
            std::stack<char> brackets_;
    };

} //namespace parser



