#include <iostream>
#include <string>

int main() {
    std::string key;
    double value;
    double sum = 0.0;
    size_t count = 0;

    while (std::cin >> key >> value) {
        sum += value;
        count += 1;
    }

    if (count == 0) {
        std::cout << "mean\t0" << std::endl;
        return 0;
    }
    std::cout << "mean\t" << (sum / count) << std::endl;
    return 0;
}
