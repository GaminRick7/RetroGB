#include "ppu.hpp"
#include <cstdio>
#include <cstring>

// ===== SPRITE LOADING =====

void PPU::pipeline_load_sprite_tile() {
    oam_line_entry* le = line_sprites;

    while (le) {
        int sp_x = (le->entry.x - 8) + (lcd->scroll_x % 8);

        if ((sp_x >= pf.fetch_x && sp_x < pf.fetch_x + 8) ||
            ((sp_x + 8) >= pf.fetch_x && (sp_x + 8) < pf.fetch_x + 8)) {
            fetched_entries[fetched_entry_count++] = le->entry;
        }

        le = le->next;
        if (!le || fetched_entry_count >= 3) {
            break;
        }
    }
}

void PPU::pipeline_load_sprite_data(u8 offset) {
    int cur_y = lcd->ly;
    u8 sprite_height = lcd->lcdc_obj_height();

    for (int i=0; i<fetched_entry_count; i++) {
        u8 ty = ((cur_y + 16) - fetched_entries[i].y) * 2;

        if (fetched_entries[i].f_y_flip) {
            //flipped upside down...
            ty = ((sprite_height * 2) - 2) - ty;
        }

        u8 tile_index = fetched_entries[i].tile;

        if (sprite_height == 16) {
            tile_index &= ~(1); //remove last bit...
        }

        pf.fetch_entry_data[(i * 2) + offset] = 
            bus->read(0x8000 + (tile_index * 16) + ty + offset);
    }
}

// ===== SPRITE PIXEL FETCHING =====

u32 PPU::fetch_sprite_pixels(u8 bit, u32 color, u8 bg_color) {
    for (int i = 0; i < fetched_entry_count; i++) {
        int sp_x = (fetched_entries[i].x - 8) + ((lcd->scroll_x % 8));
        
        if (sp_x + 8 < pf.fifo_x) {
            //past pixel point already...
            continue;
        }

        int offset = pf.fifo_x - sp_x;

        if (offset < 0 || offset > 7) {
            //out of bounds..
            continue;
        }

        bit = (7 - offset);

        if (fetched_entries[i].f_x_flip) {
            bit = offset;
        }

        u8 hi = !!(pf.fetch_entry_data[i * 2] & (1 << bit));
        u8 lo = !!(pf.fetch_entry_data[(i * 2) + 1] & (1 << bit)) << 1;

        bool bg_priority = fetched_entries[i].f_bgp;

        if (!(hi|lo)) {
            //transparent
            continue;
        }

        if (!bg_priority || bg_color == 0) {
            color = (fetched_entries[i].f_pn) ? 
                lcd->sp2_colors[hi|lo] : lcd->sp1_colors[hi|lo];

            if (hi|lo) {
                break;
            }
        }
    }

    return color;
}

// ===== SPRITE LINE LOADING =====

void PPU_SM::load_line_sprites() {
    int cur_y = ppu->lcd->ly;

    u8 sprite_height = ppu->lcd->lcdc_obj_height();
    memset(ppu->line_entry_array, 0, sizeof(ppu->line_entry_array));

    for (int i = 0; i < 40; i++) {
        oam_entry e = ppu->oam[i];
        if (!e.x) {
            //x = 0 means not visible
            continue;
        }

        if(ppu->line_sprite_count >= 10) {
            break;
        }

        if (e.y <= cur_y + 16 && e.y + sprite_height > cur_y + 16) {
            oam_line_entry* entry = &ppu->line_entry_array[ppu->line_sprite_count]; 
            entry->entry = e;
            entry->next = nullptr;
            ppu->line_sprite_count++;

            // Insert sprite into sorted linked list based on X coordinate (priority)
            // Lower X coordinates have higher priority (appear in front)
            
            // Case 1: Empty list or new sprite has highest priority (lowest X)
            if (!ppu->line_sprites || ppu->line_sprites->entry.x > e.x) {
                entry->next = ppu->line_sprites;
                ppu->line_sprites = entry;
            } else {
                // Case 2: Insert into middle or end of list
                oam_line_entry* current = ppu->line_sprites;
                oam_line_entry* previous = nullptr;
                
                // Find the correct position to insert
                while (current != nullptr) {
                    // Insert before current sprite if new sprite has higher priority
                    if (current->entry.x > e.x) {
                        entry->next = current;
                        if (previous == nullptr) {
                            // Inserting at head (shouldn't happen due to Case 1 check)
                            ppu->line_sprites = entry;
                        } else {
                            previous->next = entry;
                        }
                        break;
                    }
                    
                    // Move to next sprite
                    previous = current;
                    current = current->next;
                }
                
                // Case 3: Insert at end of list (new sprite has lowest priority)
                if (current == nullptr) {
                    previous->next = entry;
                }
            }
        }   
    }
} 