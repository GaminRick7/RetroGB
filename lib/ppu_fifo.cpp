#include "ppu.hpp"
#include <cstdio>
#include <cstdlib>

// ===== PIXEL FIFO OPERATIONS =====

void PPU::pixel_fifo_push(u32 value) {
    fifo_entry* entry = new fifo_entry;
    entry->value = value;
    entry->next = nullptr;

    if (pf.pixel_fifo.head == nullptr) {
        pf.pixel_fifo.head = pf.pixel_fifo.tail = entry;
    } else {
        pf.pixel_fifo.tail->next = entry;
        pf.pixel_fifo.tail = entry;
    }
    pf.pixel_fifo.size++;
}

u32 PPU::pixel_fifo_pop() {
    if (pf.pixel_fifo.size <= 0) {
        fprintf(stderr, "ERR IN PIXEL FIFO!\n");
        exit(-8);
    }
    fifo_entry* popped = pf.pixel_fifo.head;
    u32 value = popped->value;
    pf.pixel_fifo.head = popped->next;
    delete popped;
    pf.pixel_fifo.size--;
    return value;
}

void PPU::pipeline_fifo_reset() {
    while (pf.pixel_fifo.size > 0) {
        pixel_fifo_pop();
    }
    pf.pixel_fifo.head = nullptr;
}

bool PPU::pipeline_fifo_add() {
    if (pf.pixel_fifo.size > 8) {
        //fifo is full!
        return false;
    }

    int x = pf.fetch_x - (8 - (lcd->scroll_x % 8));

    for (int i=0; i<8; i++) {
        int bit = 7 - i;
        u8 hi = !!(pf.bgw_fetch_data[1] & (1 << bit));
        u8 lo = !!(pf.bgw_fetch_data[2] & (1 << bit)) << 1;
        u32 color = lcd->bg_colors[hi | lo];

        if (!lcd->lcdc_bgw_enable()) {
            color = lcd->bg_colors[0];
        }

        if (lcd->lcdc_obj_enable()) {
            color = fetch_sprite_pixels(bit, color, hi | lo);
        }
        if (x >= 0) {
            pixel_fifo_push(color);
            pf.fifo_x++;
        }
    }

    return true;
} 