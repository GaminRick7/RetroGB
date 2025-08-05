#include "lcd.hpp"
#include "dma.hpp"

constexpr unsigned long colors_default[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

void LCD::init() {
    lcdc = 0x91;
    scroll_x = 0;
    scroll_y = 0;
    ly = 0;
    ly_compare = 0;
    bg_palette = 0xFC;
    obj_palette[0] = 0xFF;
    obj_palette[1] = 0xFF;
    win_y = 0;
    win_x = 0;

    for (int i=0; i<4; i++) {
        bg_colors[i] = colors_default[i];
        sp1_colors[i] = colors_default[i];
        sp2_colors[i] = colors_default[i];
    }
}

void LCD::update_palette(u8 palette_data, u8 pal) {
    u32 *p_colors = bg_colors;

    switch(pal) {
        case 1:
            p_colors = sp1_colors;
            break;
        case 2:
            p_colors = sp2_colors;
            break;
    }

    p_colors[0] = colors_default[palette_data & 0b11];
    p_colors[1] = colors_default[(palette_data >> 2) & 0b11];
    p_colors[2] = colors_default[(palette_data >> 4) & 0b11];
    p_colors[3] = colors_default[(palette_data >> 6) & 0b11];
}

u8 LCD::read(u16 address) {
    u8 offset = (address - 0xFF40);
    u8 *p = (u8 *)this;

    return p[offset];
}

void LCD::write(u16 address, u8 value) {
    u8 offset = (address - 0xFF40);
    u8 *p = (u8 *)this;

    p[offset] = value;


    if (offset == 6) { 
        //0xFF46 = DMA
        dma_controller->start(value);
    }

    if (address == 0xFF47) {
        update_palette(value, 0);
    } else if (address == 0xFF48) {
        update_palette(value & 0b11111100, 1);
    } else if (address == 0xFF49) {
        update_palette(value & 0b11111100, 2);
    }
}

bool LCD::lcds_stat_int(STAT_FLAGS flag) const {
    return lcds & flag;
}