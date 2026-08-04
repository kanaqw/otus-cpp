#include <iostream>
#include <string>

int main() {
    std::string key;
    double value;
    double sum = 0.0;
    double sum_sq = 0.0;
    size_t count = 0;

    while (std::cin >> key >> value) {
        sum += value;
        sum_sq += value * value;
        count += 1;
    }

    if (count == 0) {
        std::cout << "variance\t0" << std::endl;
        return 0;
    }
    double mean = sum / count;
    double variance = sum_sq / count - mean * mean;  // E[X^2] - E[X]^2
    std::cout << "variance\t" << variance << std::endl;
    return 0;
}
