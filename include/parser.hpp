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
                            time_t time) override;
    };

    class FileLogger : public IOListener {
        public:
        void update(const std::vector<std::string>& block,
                            time_t time) override ;
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



