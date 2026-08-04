#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <vector>

inline std::vector<std::string> split(const std::string& line, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

inline std::optional<double> extract_price(const std::string& line) {
    auto tokens = split(line, ',');
    if (tokens.size() < 7) {
        return std::nullopt;
    }
    const std::string& price_str = tokens[tokens.size() - 7];
    try {
        return std::stod(price_str);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
