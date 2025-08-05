#include "dma.hpp"
#include "bus.hpp"
#include "ppu.hpp"
#include <unistd.h>

void DMA::start(u8 start) {
    active = true;
    byte = 0;
    start_delay = 2;
    value = start;
}

void DMA::set_ppu(PPU* ppu) {
    this->ppu = ppu;
}

void DMA::set_bus(Bus* bus) {
    this->bus = bus;
}

void DMA::tick() {
    if (!active) {
        return;
    }

    if (start_delay) {
        start_delay--;
        return;
    }

    ppu->oam_write(byte, bus->read(value* 0x100 + byte));

    byte++;
    active = byte < 0xA0;
}

bool DMA::transferring() {
    return active;
}