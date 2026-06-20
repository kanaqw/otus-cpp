#pragma once
#include "common.hpp"
#include "shapes.hpp"
#include "observer.hpp"

class Document {
    public:
        void add_observer(Observer& observer) {
            observers_.push_back(std::ref(observer));
        }
        void notify_observers() {
            for (auto& observer : observers_) {
                observer.get().update();
            }
        }
        void add_shape(std::unique_ptr<Shape> shape) {
            if (shape){
                shape->set_index(shapes_counter_);
                shapes_[shape->get_index()] = std::move(shape);
            }
            shapes_counter_++;
            notify_observers();
        }

        void remove_shape(int index) {
            shapes_.erase(index);
            notify_observers();
        }

        Shape& get_shape(int index) {
            auto it = shapes_.find(index);
            if (it != shapes_.end()) {
                return *it->second;
            }
            throw std::runtime_error("Shape not found");
        }

        const std::map<int, std::unique_ptr<Shape>>& get_all_shapes() const {
            return shapes_;
        }
        int get_shapes_counter() const {
            return shapes_counter_;
        }


    private:
        std::map<int, std::unique_ptr<Shape>> shapes_;
        int shapes_counter_{};
        std::vector<std::reference_wrapper<Observer>> observers_;

};
