#include "ppu.hpp"
#include "cart.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

// ===== CONSTRUCTORS & DESTRUCTORS =====

PPU::PPU() {
    // Initialize PPU state
}

PPU::~PPU() {
    // Cleanup if needed
}

// ===== INITIALIZATION =====

void PPU::init() {
    // Initialize PPU registers and state
    current_frame = 0;
    line_ticks = 0;
    video_buffer = static_cast<u32*>(malloc(YRES * XRES * sizeof(u32)));

    pf.line_x = 0;
    pf.pushed_x = 0;
    pf.fetch_x = 0;
    pf.pixel_fifo.head = nullptr;
    pf.pixel_fifo.tail = nullptr;
    pf.pixel_fifo.size = 0;
    pf.cur_fetch_state = FS_TILE;

    window_line = 0;

    line_sprites = 0;
    fetched_entry_count = 0;

    lcd->init();
    lcd->lcds_mode_set(MODE_OAM);

    // Initialize oam entries properly
    for (int i = 0; i < 40; i++) {
        oam[i] = {0, 0, 0, 0};
    }
    memset(oam, 0, sizeof(oam));
    memset(video_buffer, 0, YRES * XRES * sizeof(u32));

    ppu_sm.set_ppu(this);
    ppu_sm.set_cpu(cpu);
}

// ===== MAIN EXECUTION =====

void PPU::tick() {
    // Implement PPU tick logic
    line_ticks++;

    switch(lcd->lcds_mode()) {
    case MODE_OAM:
        ppu_sm.mode_oam();
        break;
    case MODE_XFER:
        ppu_sm.mode_xfer();
        break;
    case MODE_VBLANK:
        ppu_sm.mode_vblank();
        break;
    case MODE_HBLANK:
        ppu_sm.mode_hblank();
        break;
    }
}