#pragma once

#include "common.hpp"
#include "ram.hpp"

// Forward declaration
class Cartridge;
class IO;
class CPU;
class PPU;

class Bus {
private:
    Cartridge* cartridge;
    RAM* ram;
    CPU* cpu;
    IO* io;
    PPU* ppu;
public:
    
    void set_cartridge(Cartridge* cart);
    void set_ram(RAM* ram);
    void set_cpu(CPU* cpu);
    void set_io(IO* io);
    void set_ppu(PPU* ppu);
    u8 read(u16 address);
    void write(u16 address, u8 value);
    void write16(u16 address, u16 value);
    u16 read16(u16 address);
}; 