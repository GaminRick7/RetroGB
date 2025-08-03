#pragma once

#include "common.hpp"
struct oam_entry {
    u8 y;
    u8 x;
    u8 tile;
    u8 flags;

    unsigned f_cgb_pn : 3;
    unsigned f_cb_vram_bank : 1;
    unsigned f_cb_pn : 1;
    unsigned f_x_flip : 1;
    unsigned f_y_flip : 1;
    unsigned f_bgp : 1;

};
// Priority: 0 = No, 1 = BG and Window color indices 1–3 are drawn over this OBJ
// Y flip: 0 = Normal, 1 = Entire OBJ is vertically mirrored
// X flip: 0 = Normal, 1 = Entire OBJ is horizontally mirrored
// DMG palette [Non CGB Mode only]: 0 = OBP0, 1 = OBP1
// Bank [CGB Mode Only]: 0 = Fetch tile from VRAM bank 0, 1 = Fetch tile from VRAM bank 1
// CGB palette [CGB Mode Only]: Which of OBP0–7 to use
class PPU {
public:
    PPU();
    ~PPU();
    oam_entry oam[40];
    u8 vram[0x2000];
    void init();
    void tick();

    void oam_write(u16 address, u8 value);
    u8 oam_read(u16 address);
    void vram_write(u16 address, u8 value);
    u8 vram_read(u16 address);
    void lcd_write(u16 address, u8 value);
    // u8 lcd_read(u16 address);
    // void stat_write(u16 address, u8 value);
    // u8 stat_read(u16 address);
    // void ly_write(u16 address, u8 value);
}; 