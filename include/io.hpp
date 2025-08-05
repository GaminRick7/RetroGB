#pragma once

#include "common.hpp"
#include "timer.hpp"
#include "cpu.hpp"
#include "dma.hpp"
#include "lcd.hpp"
#include "joypad.hpp"

class IO{
    public:
        Timer* timer;
        CPU* cpu;
        DMA* dma;
        LCD* lcd;
        Joypad* joypad;
        char serial_data[2];
        u8 ly = 0;

        u8 io_read(u16 address);
        void io_write(u16 address, u8 value);
        void set_timer(Timer* t) { timer = t; }
        void set_cpu(CPU* c) { cpu = c; }
        void set_dma(DMA* d) { dma = d; }
        void set_lcd(LCD* l) { lcd = l; }
        void set_joypad(Joypad* j) { joypad = j; }
};