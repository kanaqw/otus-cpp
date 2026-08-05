// Fashion MNIST inference: logistic regression (one-vs-rest, 10 classes).
//
// Usage:
//   ./fashion_mnist <test.csv> <logreg_coef.txt>
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <numeric>
#include <limits>

namespace {

constexpr int kNumPixels = 784;

struct Sample {
    int label;
    std::vector<double> pixels;
};

using Model = std::vector<std::vector<double>>;


Model loadModel(const std::string& path) {
    Model model;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file \n");
    } 
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::vector<double> value;
        value.resize(kNumPixels + 1);

        for (size_t i = 0; i <= kNumPixels; i++){
            iss >> value[i];
        }

        model.push_back(std::move(value));
    }

    return model;
}

std::vector<Sample> loadTestData(const std::string& path) {
    std::vector<Sample> samples;

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to test data \n");
    } 
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;

        std::getline(iss, token, ',');
        int label = std::stoi(token);

        std::vector<double> pixels;
        pixels.reserve(kNumPixels);
        while (std::getline(iss, token, ',')) {
            pixels.push_back(std::stod(token));
        }

        samples.push_back({label, std::move(pixels)});
    }

    return samples;
}

int predict(const Model& model, const std::vector<double>& pixels) {
    int bestClass = 0;
    double bestScore = std::numeric_limits<double>::lowest();

    for (int c = 0; c < static_cast<int>(model.size()); ++c) {
        double score = model[c][0];
        for (int i = 0; i < kNumPixels; ++i) {
            score += model[c][i + 1] * pixels[i];
        }

        if (score > bestScore) {
            bestScore = score;
            bestClass = c;
        }
    }

    return bestClass;
}

double computeAccuracy(const Model& model, const std::vector<Sample>& samples) {
    int count = 0;
    for (auto& sample : samples){
        int class_score = predict(model, sample.pixels);
        if (class_score == sample.label){
            count++;
        }
    }
    double acc = static_cast<double>(count)/samples.size();
    
    return acc;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <test.csv> <model_file>\n";
        return 1;
    }

    const std::string testPath = argv[1];
    const std::string modelPath = argv[2];
    Model model{};

    try {
        model = loadModel(modelPath);
    } catch (const std::runtime_error& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    const std::vector<Sample> samples = loadTestData(testPath);

    const double accuracy = computeAccuracy(model, samples);

    std::cout << accuracy << std::endl;

    return 0;
}
