#pragma once
#include "common.hpp"

class Shape {
    public:
        virtual ~Shape() = default;
        virtual void draw() const = 0;
        void set_index(int index) {
            this->index = index;
        }
        int get_index() const {
            return index;
        }
    private:
        int index{};

};

class Circle : public Shape {
        void draw() const override {
            std::cout << "Drawing a circle" << std::endl;
        }
};
