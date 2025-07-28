#include "bus.hpp"
#include "cart.hpp"
#include "cpu.hpp"

// 0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
// 4000	7FFF	16 KiB ROM Bank 01–NN	From cartridge, switchable bank via mapper (if any)
// 8000	9FFF	8 KiB Video RAM (VRAM)	In CGB mode, switchable bank 0/1
// A000	BFFF	8 KiB External RAM	From cartridge, switchable bank if any
// C000	CFFF	4 KiB Work RAM (WRAM)	
// D000	DFFF	4 KiB Work RAM (WRAM)	In CGB mode, switchable bank 1–7
// E000	FDFF	Echo RAM (mirror of C000–DDFF)	Nintendo says use of this area is prohibited.
// FE00	FE9F	Object attribute memory (OAM)	
// FEA0	FEFF	Not Usable	Nintendo says use of this area is prohibited.
// FF00	FF7F	I/O Registers	
// FF80	FFFE	High RAM (HRAM)	
// FFFF	FFFF	Interrupt Enable register (IE)	


void Bus::set_cartridge(Cartridge* cart) {
    cartridge = cart;
}

void Bus::set_ram(RAM* ram) {
    this->ram = ram;
}

void Bus::set_cpu(CPU* cpu) {
    this->cpu = cpu;
}

u8 Bus::read(u16 address) {
    if (address < 8000) {
        return cartridge->read(address);
    }
    else if (address >= 0xC000 && address <= 0xDFFF) {
        return ram->read_wram(address);
    }
    else if (address >= 0xFF80 && address <= 0xFFFE) {
        return ram->read_hram(address);
    }
    // Implement memory read logic
    printf("Reading from unimplemented address %04X\n", address);
    return 0;
}

void Bus::write(u16 address, u8 value) {
    // Implement memory write logic
    if (address < 8000) {
        cartridge->write(address, value);
    }
    else if (address >= 0xC000 && address <= 0xDFFF) {
        ram->write_wram(address, value);
    }
    else if (address >= 0xFF80 && address <= 0xFFFE) {
        ram->write_hram(address, value);
    }
    else if (address == 0xFFFF) {
        cpu->set_ie_register(value);
    }
    else{
        printf("\nWriting to unimplemented address %04X\n", address);
    }
    
}

void Bus::write16(u16 addr, u16 value) {
    write(addr, value & 0xFF);         // low byte
    write(addr + 1, (value >> 8) & 0xFF); // high byte
}

u16 Bus::read16(u16 addr) {
    u8 lo = read(addr);
    u8 hi = read(addr + 1);
    return lo | (hi << 8);
} 