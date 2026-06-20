#pragma once
#include "common.hpp"

class Observer {
    public:
        virtual ~Observer() = default;
        virtual void update() = 0;
};
