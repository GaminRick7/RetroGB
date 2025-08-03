#pragma once

#include "common.hpp"
#include "timer.hpp"
#include "cpu.hpp"

class IO{
    public:
        Timer* timer;
        CPU* cpu;
        char serial_data[2];

        u8 io_read(u16 address);
        void io_write(u16 address, u8 value);
        void set_timer(Timer* t) { timer = t; }
        void set_cpu(CPU* c) { cpu = c; }
};