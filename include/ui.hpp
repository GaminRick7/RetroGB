#pragma once

#include "common.hpp"
#include "bus.hpp"
#include "joypad.hpp"
#include <SDL.h>
#include <atomic>

/**
 * @brief User interface and rendering system
 * 
 * Handles all SDL-based rendering and user interaction:
 * - Window management and rendering
 * - Event handling (keyboard, mouse, window events)
 * - Debug window for tile/sprite viewing
 * - Frame rate limiting and timing
 * - Input processing for joypad
 */
class UI {
public:
    // ===== CONSTRUCTORS & DESTRUCTORS =====
    UI();
    ~UI();
    
    // ===== INITIALIZATION & CLEANUP =====
    bool init();
    void cleanup();
    
    // ===== COMPONENT CONNECTIONS =====
    void set_bus(Bus* b) { bus = b; }
    void set_ppu(PPU* p) { ppu = p; }
    void set_joypad(Joypad* j) { joypad = j; }
    
    // ===== WINDOW MANAGEMENT =====
    SDL_Window* get_window() const { return window; }
    SDL_Renderer* get_renderer() const { return renderer; }
    SDL_Window* get_debug_window() const { return debug_window; }
    SDL_Renderer* get_debug_renderer() const { return debug_renderer; }
    
    // ===== DEBUG MODE CONTROL =====
    void set_debug_enabled(bool enabled) { debug_enabled = enabled; }
    bool is_debug_enabled() const { return debug_enabled; }
    bool is_initialized() const;
    
    // ===== RENDERING & DISPLAY =====
    void update();
    void update_debug_window();
    void display_tile(SDL_Surface* surface, u16 addr, u16 tileNum, int x, int y);
    void render_frame();
    void limit_frame_rate(int target_fps);
    
    // ===== EVENT HANDLING =====
    bool handle_events(std::atomic<bool>& running, std::atomic<bool>& paused);
    
    // ===== UTILITY =====
    void delay(u32 ms);

private:
    // ===== SDL RESOURCES =====
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Surface* screen;
    
    // ===== DEBUG WINDOW RESOURCES =====
    SDL_Window* debug_window;
    SDL_Renderer* debug_renderer;
    SDL_Texture* debug_texture;
    SDL_Surface* debug_screen;
    
    // ===== COMPONENT REFERENCES =====
    Bus* bus;
    PPU* ppu;
    Joypad* joypad;
    
    // ===== STATE =====
    bool initialized;
    bool debug_enabled;
    int scale;
    
    // ===== RENDERING CONSTANTS =====
    unsigned long tile_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};
}; 