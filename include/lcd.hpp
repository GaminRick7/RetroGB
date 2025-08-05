#pragma once

#include "common.hpp"

class DMA;

enum LCD_MODES {
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_XFER
};

enum STAT_FLAGS {
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM = (1 << 5),
    SS_LYC = (1 << 6),
};

class LCD {
    public:
        //registers...
        u8 lcdc;
        u8 lcds;
        u8 scroll_y;
        u8 scroll_x;
        u8 ly;
        u8 ly_compare;
        u8 dma;
        u8 bg_palette;
        u8 obj_palette[2];
        u8 win_y;
        u8 win_x;

        DMA* dma_controller;

        //other data...
        u32 bg_colors[4];
        u32 sp1_colors[4];
        u32 sp2_colors[4];

        // LCD Control Register (LCDC) getters
        bool lcdc_bgw_enable() const { return BIT(lcdc, 0); }
        bool lcdc_obj_enable() const { return BIT(lcdc, 1); }
        u8 lcdc_obj_height() const { return BIT(lcdc, 2) ? 16 : 8; }
        u16 lcdc_bg_map_area() const { return BIT(lcdc, 3) ? 0x9C00 : 0x9800; }
        u16 lcdc_bgw_data_area() const { return BIT(lcdc, 4) ? 0x8000 : 0x8800; }
        bool lcdc_win_enable() const { return BIT(lcdc, 5); }
        u16 lcdc_win_map_area() const { return BIT(lcdc, 6) ? 0x9C00 : 0x9800; }
        bool lcdc_lcd_enable() const { return BIT(lcdc, 7); }

        // LCD Status Register (LCDS) getters
        LCD_MODES lcds_mode() const { return static_cast<LCD_MODES>(lcds & 0b11); }
        void lcds_mode_set(LCD_MODES mode) { lcds &= ~0b11; lcds |= mode; }
        bool lcds_lyc() const { return BIT(lcds, 2); }
        void lcds_lyc_set(bool b) { BIT_SET(lcds, 2, b); }
        bool lcds_stat_int(STAT_FLAGS flag) const;

        void init();
        void update_palette(u8 palette_data, u8 pal);

        u8 read(u16 address);
        void write(u16 address, u8 value);

        void set_dma(DMA* d) { dma_controller = d; }
};
