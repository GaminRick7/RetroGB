#include "ppu.hpp"
#include <cstdio>

// ===== OAM OPERATIONS =====

void PPU::oam_write(u16 address, u8 value) {
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }
    // Convert oam to a byte array for direct access
    u8* oam_bytes = reinterpret_cast<u8*>(oam);
    oam_bytes[address] = value;
}

u8 PPU::oam_read(u16 address) {
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }
    // Convert oam to a byte array for direct access
    u8* oam_bytes = reinterpret_cast<u8*>(oam);
    return oam_bytes[address];
}

// ===== VRAM OPERATIONS =====

void PPU::vram_write(u16 address, u8 value) {
    vram[address - 0x8000] = value;
}

u8 PPU::vram_read(u16 address) {
    return vram[address - 0x8000];
}

// ===== LCD OPERATIONS =====

void PPU::lcd_write(u16 address, u8 value) {
    // LCD register write operations
    // This function is currently empty but can be expanded for LCD register handling
} 