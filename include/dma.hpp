#pragma once

#include "common.hpp"
#include "ppu.hpp"
#include "bus.hpp"

class Bus;

class DMA {
    public:
        void start(u8 start);
        void tick();
        bool transferring();
        void set_ppu(PPU* ppu);
        void set_bus(Bus* bus);
    private:
        bool active;
        u8 byte;
        u8 value;
        u8 start_delay;
        PPU* ppu;
        Bus* bus;
};