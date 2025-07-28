#include "ram.hpp"


u8 RAM::read_wram(u16 address){
    return wram[address - 0xC000];
}

u8 RAM::read_hram(u16 address){
    return hram[address - 0xFF80];
}
void RAM::write_wram(u16 address, u8 value){
    wram[address - 0xC000] = value;
}

void RAM::write_hram(u16 address, u8 value){
    printf("Writing to HRAM %04X = %02X\n", address, value);
    hram[address - 0xFF80] = value;
}