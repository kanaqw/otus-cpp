#include <iostream>
#include <string>

#include "include/common.hpp"

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (auto price = extract_price(line)) {
            std::cout << "price\t" << *price << "\n";
        }
    }
    return 0;
}
