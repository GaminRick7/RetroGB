#include "ppu.hpp"
#include <cstdio>

// ===== PIPELINE PROCESSING =====

void PPU::pipeline_process() {
    pf.map_y = lcd->ly + lcd->scroll_y;
    pf.map_x = pf.fetch_x + lcd->scroll_x;
    pf.tile_y = ((lcd->ly + lcd->scroll_y) % 8) * 2;

    if (!(line_ticks & 1)) {
        pipeline_fetch();
    }

    pipeline_push_pixel();
}

void PPU::pipeline_push_pixel() {
    if (pf.pixel_fifo.size > 8) {
        u32 pixel_data = pixel_fifo_pop();

        if (pf.line_x >= (lcd->scroll_x % 8)) {
            video_buffer[pf.pushed_x + (lcd->ly * XRES)] = pixel_data;
            pf.pushed_x++;
        }
        pf.line_x++;
    }
}

// ===== PIPELINE FETCHING =====

void PPU::pipeline_fetch() {
    switch(pf.cur_fetch_state) {
        case FS_TILE: {
            fetched_entry_count = 0;

            if (lcd->lcdc_bgw_enable()) {
                pf.bgw_fetch_data[0] = bus->read(lcd->lcdc_bg_map_area() + 
                    (pf.map_x / 8) + 
                    (((pf.map_y / 8)) * 32));
            
                if (lcd->lcdc_bgw_data_area() == 0x8800) {
                    pf.bgw_fetch_data[0] += 128;
                }

                pipeline_load_window_tile();
            }
            if (lcd->lcdc_obj_enable() && line_sprites) {
                pipeline_load_sprite_tile();
            }

            pf.cur_fetch_state = FS_DATA0;
            pf.fetch_x += 8;
        } break;

        case FS_DATA0: {
            pf.bgw_fetch_data[1] = bus->read(lcd->lcdc_bgw_data_area() +
                (pf.bgw_fetch_data[0] * 16) + 
                pf.tile_y);
            
            pipeline_load_sprite_data(0);

            pf.cur_fetch_state = FS_DATA1;
        } break;

        case FS_DATA1: {
            pf.bgw_fetch_data[2] = bus->read(lcd->lcdc_bgw_data_area() +
                (pf.bgw_fetch_data[0] * 16) + 
                pf.tile_y + 1);

            pipeline_load_sprite_data(1);

            pf.cur_fetch_state = FS_IDLE;

        } break;

        case FS_IDLE: {
            pf.cur_fetch_state = FS_PUSH;
        } break;

        case FS_PUSH: {
            if (pipeline_fifo_add()) {
                pf.cur_fetch_state = FS_TILE;
            }

        } break;
    }
}

// ===== WINDOW TILE LOADING =====

void PPU::pipeline_load_window_tile() {
    if (!window_visible()) {
        return;
    }
    
    u8 window_y = lcd->win_y;

    if (pf.fetch_x + 7 >= lcd->win_x &&
            pf.fetch_x + 7 < lcd->win_x + YRES + 14) {
        if (lcd->ly >= window_y && lcd->ly < window_y + XRES) {
            u8 w_tile_y = window_line / 8;

            pf.bgw_fetch_data[0] = bus->read(lcd->lcdc_win_map_area() + 
                ((pf.fetch_x + 7 - lcd->win_x) / 8) +
                (w_tile_y * 32));

            if (lcd->lcdc_bgw_data_area() == 0x8800) {
                pf.bgw_fetch_data[0] += 128;
            }
        }
    }
}

// ===== WINDOW VISIBILITY CHECK =====

bool PPU::window_visible() {
    return lcd->lcdc_win_enable() && lcd->win_x >= 0 && lcd->win_x <= 166 && lcd->win_y >= 0 && lcd->win_y < YRES;
} 