#pragma once

#include "common.hpp"
#include "lcd.hpp"
#include "ppu_sm.hpp"
#include "bus.hpp"

class CPU;
class Bus;
class Cartridge;

// ===== PPU CONSTANTS =====
constexpr int LINES_PER_FRAME = 154;
constexpr int TICKS_PER_LINE = 456;
constexpr int YRES = 144;
constexpr int XRES = 160;

/**
 * @brief Object Attribute Memory entry
 * 
 * Represents a single sprite in OAM memory with:
 * - Position (x, y coordinates)
 * - Tile number
 * - Attributes (priority, flip, palette, etc.)
 */
struct oam_entry {
    u8 y;                    // Y position
    u8 x;                    // X position
    u8 tile;                 // Tile number
    u8 f_cgb_pn : 3;        // CGB palette number
    u8 f_cgb_vram_bank : 1; // CGB VRAM bank
    u8 f_pn : 1;            // Priority number
    u8 f_x_flip : 1;        // X flip flag
    u8 f_y_flip : 1;        // Y flip flag
    u8 f_bgp : 1;           // Background priority
};

/**
 * @brief Linked list entry for OAM line sprites
 */
struct oam_line_entry {
    oam_entry entry;
    oam_line_entry* next;
};

/**
 * @brief Pixel FIFO fetch states
 */
enum fetch_state {
    FS_TILE,   // Fetching tile number
    FS_DATA0,  // Fetching tile data low byte
    FS_DATA1,  // Fetching tile data high byte
    FS_IDLE,   // Idle state
    FS_PUSH,   // Pushing pixels to FIFO
};

/**
 * @brief FIFO entry for pixel data
 */
struct fifo_entry {
    u32 value;
    fifo_entry* next;
};

/**
 * @brief Pixel FIFO queue
 */
struct fifo {
    fifo_entry* head;
    fifo_entry* tail;
    u32 size;
};

/**
 * @brief Pixel FIFO with fetch state
 */
struct pixel_fifo {
    fetch_state cur_fetch_state;
    fifo pixel_fifo;
    u8 line_x;
    u8 pushed_x;
    u8 fetch_x;
    u8 bgw_fetch_data[3];
    u8 fetch_entry_data[6]; // OAM data
    u8 map_y;
    u8 map_x;
    u8 tile_y;
    u8 fifo_x;
};

/**
 * @brief Fetch context for tile data
 */
struct fetch_ctx {
    u8* tile_data;
    u8* tile_map;
};

/**
 * @brief Picture Processing Unit (PPU)
 * 
 * Handles all graphics rendering including:
 * - Background and window rendering
 * - Sprite rendering and OAM management
 * - Pixel FIFO and fetch pipeline
 * - VRAM and OAM memory management
 * - LCD timing and synchronization
 */
class PPU {
public:
    // ===== CONSTRUCTORS & DESTRUCTORS =====
    PPU();
    ~PPU();
    
    // ===== INITIALIZATION =====
    void init();
    
    // ===== MAIN EXECUTION =====
    void tick();
    
    // ===== COMPONENT CONNECTIONS =====
    void set_cpu(CPU* c) { cpu = c; }
    void set_lcd(LCD* l) { lcd = l; }
    void set_bus(Bus* b) { bus = b; }
    void set_cart(Cartridge* c) { cart = c; }
    
    // ===== MEMORY ACCESS =====
    void oam_write(u16 address, u8 value);
    u8 oam_read(u16 address);
    void vram_write(u16 address, u8 value);
    u8 vram_read(u16 address);
    void lcd_write(u16 address, u8 value);
    
    // ===== PIXEL FIFO OPERATIONS =====
    void pixel_fifo_push(u32 value);
    u32 pixel_fifo_pop();
    
    // ===== PIPELINE OPERATIONS =====
    void pipeline_fetch();
    void pipeline_process();
    void pipeline_push_pixel();
    bool pipeline_fifo_add();
    void pipeline_fifo_reset();
    void pipeline_load_sprite_tile();
    void pipeline_load_sprite_data(u8 offset);
    void pipeline_load_window_tile();
    
    // ===== SPRITE OPERATIONS =====
    u32 fetch_sprite_pixels(u8 bit, u32 color, u8 bg_color);
    
    // ===== WINDOW OPERATIONS =====
    bool window_visible();

    // ===== PUBLIC MEMBERS FOR EMULATOR ACCESS =====
    u32 current_frame;
    LCD* lcd;
    CPU* cpu;
    u8 window_line;
    u8 line_sprite_count;
    oam_line_entry line_entry_array[10];
    oam_entry oam[40];
    oam_line_entry* line_sprites;
    u32 line_ticks;
    pixel_fifo pf;
    Cartridge* cart;
    u32* video_buffer;

private:
    // ===== COMPONENT REFERENCES =====
    PPU_SM ppu_sm;
    Bus* bus;
    
    // ===== FETCH STATE =====
    u8 fetched_entry_count;
    oam_entry fetched_entries[3];
    
    // ===== MEMORY =====
    u8 vram[0x2000];
}; 