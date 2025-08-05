#include "common.hpp"
#include "SDL.h"
#include "ppu_sm.hpp"
#include "ppu.hpp"
#include "cpu.hpp"
#include "cart.hpp"

// ===== LINE INCREMENT =====

void PPU_SM::increment_ly() {
    if (ppu->window_visible() && ppu->lcd->ly >= ppu->lcd->win_y && ppu->lcd->ly < ppu->lcd->win_y + YRES) {
        ppu->window_line++;
    }
    ppu->lcd->ly++;

    if (ppu->lcd->ly == ppu->lcd->ly_compare) {
        ppu->lcd->lcds_lyc_set(1);

        if (ppu->lcd->lcds_stat_int(SS_LYC)) {
            ppu->cpu->request_interrupt(IT_LCD_STAT);
        }
    } else {
        ppu->lcd->lcds_lyc_set(0);
    }
}

// ===== STATE MACHINE MODES =====

void PPU_SM::mode_oam() {
    if (ppu->line_ticks >= 80) {
        ppu->lcd->lcds_mode_set(MODE_XFER);
        ppu->pf.cur_fetch_state = FS_TILE; 
        ppu->pf.line_x = 0;
        ppu->pf.pushed_x = 0;
        ppu->pf.fetch_x = 0;
        ppu->pf.fifo_x = 0;
    }

    if (ppu->line_ticks == 1) {
        ppu->line_sprites = 0;
        ppu->line_sprite_count = 0;

        load_line_sprites();
    }
}

void PPU_SM::mode_xfer() {
    ppu->pipeline_process();

    if (ppu->pf.pushed_x >= XRES) {
        ppu->pipeline_fifo_reset();
        ppu->lcd->lcds_mode_set(MODE_HBLANK);

        if (ppu->lcd->lcds_stat_int(SS_HBLANK)) {
            ppu->cpu->request_interrupt(IT_LCD_STAT);
        }
    }
}

void PPU_SM::mode_vblank() {
    if (ppu->line_ticks >= TICKS_PER_LINE) {
        increment_ly();

        if (ppu->lcd->ly >= LINES_PER_FRAME) {
            ppu->lcd->lcds_mode_set(MODE_OAM);
            ppu->lcd->ly = 0;
            ppu->window_line = 0;
        }
        ppu->line_ticks = 0;
    }
}

void PPU_SM::mode_hblank() {
    if (ppu->line_ticks >= TICKS_PER_LINE) {
        increment_ly();

        if (ppu->lcd->ly >= YRES) {
            ppu->lcd->lcds_mode_set(MODE_VBLANK);

            ppu->cpu->request_interrupt(IT_VBLANK);

            if (ppu->lcd->lcds_stat_int(SS_VBLANK)) {
                ppu->cpu->request_interrupt(IT_LCD_STAT);
            }

            ppu->current_frame++;

            //calc FPS...
            u32 end = SDL_GetTicks();
            u32 frame_time = end - prev_frame_time;

            if (frame_time < target_frame_time) {
                delay((target_frame_time - frame_time));
            }

            if (end - start_timer >= 1000) {
                u32 fps = frame_count;
                start_timer = end;
                frame_count = 0;

                printf("FPS: %d\n", fps);

                if (ppu->cart->get_need_save()) {
                    ppu->cart->battery_save();
                }
            }

            frame_count++;
            prev_frame_time = SDL_GetTicks();

        } else {
            ppu->lcd->lcds_mode_set(MODE_OAM);
        }

        ppu->line_ticks = 0;
    }
}