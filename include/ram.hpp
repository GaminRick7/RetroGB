#pragma once

#include "common.hpp"

class RAM {
private:
    // Work RAM (WRAM) - 8KB total
    // C000-CFFF: 4KB WRAM Bank 0
    // D000-DFFF: 4KB WRAM Bank 1 (switchable in CGB mode)
    u8 wram[0xDFFF - 0xC000 + 1];  // 8KB total WRAM
    
    // High RAM (HRAM) - 127 bytes
    // FF80-FFFE: High RAM
    u8 hram[0xFFFE - 0xFF80 + 1];   // 127 bytes HRAM

public:
    
    // Read from RAM
    u8 read_wram(u16 address);
    u8 read_hram(u16 address);
    
    // Write to RAM
    void write_wram(u16 address, u8 value);
    void write_hram(u16 address, u8 value);
    
    // Read/write with bounds checking
    u8 read(u16 address);
    void write(u16 address, u8 value);
}; 