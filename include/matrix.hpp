#include <iostream>
#include <map>
#include <tuple>


template <typename T, T DefVal>
class Matrix {
    public:
        class Cell_P; //forward declared
        class Row_P {
            private:
                Matrix& matrix;
                int row;
            public:
                Row_P(Matrix& matrix, int row) : matrix(matrix), row(row) {}
                Cell_P operator[](int col) {
                    return Cell_P(matrix, row, col);
                }
        };

        class Cell_P {
            private:
                Matrix& matrix;
                int row, col;
            public:
                Cell_P(Matrix& matrix, int row, int col) : matrix(matrix), row(row), col(col) {}

                Cell_P& operator=(const T& value) {
                    if (value == DefVal) {
                        matrix.data_.erase({row, col});
                    } else {
                        matrix.data_[{row, col}] = value;
                    }
                    return *this;
                }

                Cell_P& operator=(const Cell_P& other){
                    T value = static_cast<T>(other);
                    return (*this = value);
                }

                operator T() const {
                    auto it = matrix.data_.find({row, col});
                    if (it != matrix.data_.end()) {
                        return it->second;
                    }
                    return DefVal;
                }
        };

        Row_P operator[](int row) {
            return Row_P(*this, row);
        }

        std::size_t size() const {
            return data_.size();
        }

        auto begin() {
            return data_.begin();
        }

        auto end() {
            return data_.end();
        }


    private:
        using Key = std::pair<int, int>;
        std::map<Key, T> data_;
};
