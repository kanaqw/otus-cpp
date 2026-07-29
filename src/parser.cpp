#include "parser.hpp"

namespace parser {

namespace {
    std::atomic<size_t> g_file_seq{0};
}

ConsoleLogger::ConsoleLogger()
    : thread_(&ConsoleLogger::worker_loop, this) {}

ConsoleLogger::~ConsoleLogger() {
    queue_.stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void ConsoleLogger::update(const std::vector<std::string>& block,
                            time_t time) {
    queue_.push(Bulk{block, time});
}

void ConsoleLogger::worker_loop() {
    while (auto bulk = queue_.pop()) {
        if (bulk->commands.empty()) {
            continue;
        }
        std::cout << "Bulk: ";
        for (auto it = bulk->commands.begin(); const auto& cmd : bulk->commands){
            std::cout << cmd;
            if (++it != bulk->commands.end()){
                std::cout << ",";
            }
        }
        std::cout << "\n";
    }
}

FileLogger::FileLogger()
    : file_1_(&FileLogger::worker_loop, this, std::ref(queue1_)),
      file_2_(&FileLogger::worker_loop, this, std::ref(queue2_)) {}

FileLogger::~FileLogger() {
    queue1_.stop();
    queue2_.stop();
    if (file_1_.joinable()) {
        file_1_.join();
    }
    if (file_2_.joinable()) {
        file_2_.join();
    }
}

void FileLogger::update(const std::vector<std::string>& block,
                            time_t time) {
    size_t idx = counter_.fetch_add(1, std::memory_order_relaxed) % 2;
    (idx == 0 ? queue1_ : queue2_).push(Bulk{block, time});
}

void FileLogger::worker_loop(SafeQueue<Bulk>& queue) {
    while (auto bulk = queue.pop()) {
        if (bulk->commands.empty()) {
            continue;
        }
        size_t seq = g_file_seq.fetch_add(1, std::memory_order_relaxed);
        std::string filename = "Bulk" + std::to_string(bulk->time) + "_" + std::to_string(seq) + ".log";
        std::ofstream file(filename);

        if (file.is_open()) {
            file << "bulk: ";
            for (auto it = bulk->commands.begin(); const auto& cmd : bulk->commands){
                file << cmd;
                if (++it != bulk->commands.end()){
                    file << ",";
                }
            }
            file << "\n";
        }
    }
}

void PackHandler::add_cmd_to_pack(const std::string& cmd){
   if (cmd == "{") {
          handle_open_bracket();
      } else if (cmd == "}") {
          handle_close_bracket();
      } else {
          handle_regular_cmd(cmd);
      }
}

void PackHandler::flush_eof() {
      if (currentState_ == CmdType::STATIC && !commands_.empty()){
          notify();
      }
      commands_.clear();
  }
void PackHandler::handle_open_bracket() {
      if (currentState_ == CmdType::STATIC) {
          if (!commands_.empty()) {
              notify();
          }
          currentState_ = CmdType::DYNAMIC;
      }
      brackets_.push('{');
  }

void PackHandler::handle_close_bracket(){
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

void PackHandler::handle_regular_cmd(const std::string& cmd){
      if (commands_.empty()){
          timestamp_ = std::time(nullptr);
      }
      commands_.push_back(cmd);
      if (currentState_ == CmdType::STATIC &&
          commands_.size() == packSize_){
              notify();
          }
  }

void PackHandler::notify() {
        for (auto& listener : listeners_) {
            listener->update(commands_, timestamp_);
        }
        commands_.clear();
    }
  
  

} //namespace parser

