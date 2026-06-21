#include "parser.hpp"
namespace parser {

void ConsoleLogger::update(const std::vector<std::string>& block,
                            time_t time) {
     if (!block.empty()){
          std::cout << "Bulk time:" << time << "Bulk: ";
          for (auto it = block.begin();const auto& cmd : block){
              std::cout << cmd;
              if (it!=block.end()){
                  std::cout << ",";
              }
          }
          std::cout << "\n";
    }
};

void FileLogger::update(const std::vector<std::string>& block,
                            time_t time) {
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
};

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

