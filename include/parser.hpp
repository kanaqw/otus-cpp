#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <memory>
#include <stack>

enum class CmdType : size_t {
    STATIC = 0, 
    DYNAMIC
};

namespace parser {

    class IOListener {
        public:
        virtual void update(const std::vector<std::string>& block,
                            time_t time) = 0;
        virtual ~IOListener() = default;
    };

    class ConsoleLogger : public IOListener {
        public:
        void update(const std::vector<std::string>& block,
                            time_t time) override {
            if (!block.empty()){
                std::cout << "Bulk: " ;
                for (auto it = block.begin();const auto& cmd : block){
                    std::cout << cmd;
                    if (it!=block.end()){
                        std::cout << ",";
                    }
                }
                std::cout << "\n";
            }

        }
    };

    class FileLogger : public IOListener {
        public:
        void update(const std::vector<std::string>& block,
                            time_t time) override {
            if (!block.empty()){
                std::string filename = "Bulk" + std::to_string(time) + ".log";
                std::ofstream file(filename);

                if (file.is_open()) {
                    file << "bulk: ";
                    for (auto it = block.begin();const auto& cmd : block){
                        std::cout << cmd;
                        if (it!=block.end()){
                            std::cout << ",";
                        }
                    }
                    std::cout << "\n";
                }
            }
        }
    };

    class PackHandler {
        public:
        PackHandler(size_t N) 
        : packSize_(N),
          currentState_(CmdType::STATIC),
          brackets_(),
          timestamp_(0)
          {}

        void subscribe(std::shared_ptr<IOListener> listener) {
            listeners_.push_back(listener);
        }

        void add_cmd_to_pack(const std::string& cmd){
            if (cmd == "{") {
                handle_open_bracket();
            } else if (cmd == "}") {
                handle_close_bracket();
            } else {
                handle_regular_cmd(cmd);
            }
        }

        void flush_eof() {
            if (currentState_ == CmdType::STATIC && !commands_.empty()){
                notify();
            }
            commands_.clear();
        }

        private:
            void handle_open_bracket(){
                if (currentState_ == CmdType::STATIC) {
                    if (!commands_.empty()) {
                        notify();
                    }
                    currentState_ = CmdType::DYNAMIC;
                }
                brackets_.push('{');
            }

            void handle_close_bracket(){
                if (currentState_ == CmdType::DYNAMIC){
                    if (!brackets_.empty()){
                        brackets_.pop();
                    }
                    if (brackets_.empty()){
                        notify();
                        currentState_ = CmdType::STATIC;
                    }
                }
            }

            void handle_regular_cmd(const std::string& cmd){
                if (commands_.empty()){
                    timestamp_ = std::time(nullptr);
                }
                commands_.push_back(cmd);
                if (currentState_ == CmdType::STATIC &&
                    commands_.size() == packSize_){
                        notify();
                    }
            }

            void notify() {
                for (auto& listener : listeners_) {
                    listener->update(commands_, timestamp_);
                }
                commands_.clear();
            }

            std::vector<std::string> commands_;
            std::vector<std::shared_ptr<IOListener>> listeners_;
            CmdType currentState_{};
            size_t packSize_;
            time_t timestamp_;
            std::stack<char> brackets_;
    };

} //namespace parser



