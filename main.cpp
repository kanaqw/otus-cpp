#include "include/matrix.hpp"

int main() {
    Matrix<int, 0> matrix;
    
    //main diagonal
    for (int i = 0; i < 10; i++){
        matrix[i][i] = i;
    }
    //secondary diagonal
    for (int i = 0; i < 10; i++){
        matrix[i][9-i] = 9-i;
    }

    //fragment output
    for (int row = 1; row <= 8; ++row){
        for (int col = 1; col <= 8; ++col){
            std::cout << matrix[row][col];
            if (col < 8) {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
    //number of occupied cells
    std::cout << "Occupied cells: " << matrix.size() << std::endl;

    //all stored cells
    std::cout << "Stored cells:" << std::endl;
    for (const auto& item : matrix) {
        int x = item.first.first;
        int y = item.first.second;
        int value = item.second;

        std::cout << "[" << x << "][" << y << "] = " << value << std::endl;

    }

    return 0;
}
