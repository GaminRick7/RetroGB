#pragma once

#include "common.hpp"
#include "ram.hpp"
#include "dma.hpp"

// Forward declarations
class Cartridge;
class IO;
class CPU;
class PPU;
class DMA;

/**
 * @brief Memory bus for Game Boy emulator
 * 
 * Handles all memory access routing between components:
 * - ROM access (0x0000-0x7FFF)
 * - VRAM access (0x8000-0x9FFF)
 * - WRAM access (0xC000-0xDFFF)
 * - OAM access (0xFE00-0xFE9F)
 * - I/O registers (0xFF00-0xFF7F)
 * - HRAM access (0xFF80-0xFFFE)
 * - Interrupt registers (0xFF0F, 0xFFFF)
 */
class Bus {
public:
    // ===== COMPONENT CONNECTIONS =====
    void set_cartridge(Cartridge* cart);
    void set_ram(RAM* ram);
    void set_cpu(CPU* cpu);
    void set_io(IO* io);
    void set_ppu(PPU* ppu);
    void set_dma(DMA* dma);
    
    // ===== MEMORY ACCESS =====
    u8 read(u16 address);
    void write(u16 address, u8 value);
    void write16(u16 address, u16 value);
    u16 read16(u16 address);

private:
    // ===== COMPONENT REFERENCES =====
    Cartridge* cartridge;
    RAM* ram;
    CPU* cpu;
    IO* io;
    PPU* ppu;
    DMA* dma;
}; 