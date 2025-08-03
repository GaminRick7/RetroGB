#pragma once

#include "common.hpp"

class CPU;

class Timer {
public:
    u16 div;
    u8 tma;
    u8 tac;
    u8 tima;
    CPU* cpu;
    void init();
    void tick();
    void set_cpu(CPU* c) { cpu = c; }

    void write(u16 address, u8 value);
    u8 read(u16 address);
}; 