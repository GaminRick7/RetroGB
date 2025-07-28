#pragma once

#include "common.hpp"
#include "cart.hpp"
#include "bus.hpp"
#include "cpu.hpp"
#include "ram.hpp"

class EmuContext {
public:
    bool paused;
    bool running;
    u64 ticks;
};

class Emulator {
private:
    EmuContext ctx;
    Cartridge cartridge;
    RAM ram;
    Bus bus;
    CPU cpu;

public:
    Emulator();
    ~Emulator();
    
    int run(int argc, char** argv);
    EmuContext* get_context();
}; 