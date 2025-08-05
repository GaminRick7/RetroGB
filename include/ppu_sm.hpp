#pragma once

#include "common.hpp"
#include <SDL.h>

class PPU;
class CPU;

class PPU_SM {
    public:
        PPU* ppu;
        CPU* cpu;

        u32 target_frame_time = 1000 / 60;
        long prev_frame_time = 0;
        long start_timer = 0;
        long frame_count = 0;

        void set_ppu(PPU* p) { ppu = p; }
        void set_cpu(CPU* c) { cpu = c; }
        void mode_oam();
        void mode_xfer();
        void mode_hblank();
        void mode_vblank();
        void increment_ly();
        void load_line_sprites();
};  