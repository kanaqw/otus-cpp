// csv_logger.hpp — one row per sim tick; columns fixed at construction.
#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>

namespace sim {

class CsvLogger {
public:
    CsvLogger(const std::string& path, std::vector<std::string> columns)
        : columns_(std::move(columns)) {
        out_.open(path);
        if (!out_) throw std::runtime_error("csv_logger: could not open " + path);
        for (size_t i = 0; i < columns_.size(); ++i) {
            out_ << columns_[i] << (i + 1 < columns_.size() ? "," : "\n");
        }
    }

    // values.size() must equal columns.size()
    void writeRow(const std::vector<double>& values) {
        if (values.size() != columns_.size())
            throw std::runtime_error("csv_logger: column/value count mismatch");
        for (size_t i = 0; i < values.size(); ++i) {
            out_ << values[i] << (i + 1 < values.size() ? "," : "\n");
        }
    }

private:
    std::ofstream out_;
    std::vector<std::string> columns_;
};

} // namespace sim
