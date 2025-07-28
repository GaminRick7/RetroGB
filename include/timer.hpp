#pragma once

#include "common.hpp"

class Timer {
public:
    Timer();
    ~Timer();
    
    void init();
    void tick();
}; 