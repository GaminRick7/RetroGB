#pragma once

#include "common.hpp"

class PPU {
public:
    PPU();
    ~PPU();
    
    void init();
    void tick();
}; 