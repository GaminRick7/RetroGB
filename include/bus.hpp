#pragma once

#include "common.hpp"
#include "ram.hpp"

// Forward declaration
class Cartridge;
class CPU;

class Bus {
private:
    Cartridge* cartridge;
    RAM* ram;
    CPU* cpu;

public:
    
    void set_cartridge(Cartridge* cart);
    void set_ram(RAM* ram);
    void set_cpu(CPU* cpu);
    u8 read(u16 address);
    void write(u16 address, u8 value);
    void write16(u16 address, u16 value);
    u16 read16(u16 address);
}; 