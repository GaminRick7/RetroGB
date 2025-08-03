#pragma once

#include "common.hpp"
#include <SDL.h>
#include <atomic>

class UI {
private:
    bool initialized;
    SDL_Window* window;
    SDL_Renderer* renderer;
    
public:
    UI();
    ~UI();
    
    bool init();
    void cleanup();
    void delay(u32 ms);
    bool is_initialized() const;
    
    // Window management
    SDL_Window* get_window() const { return window; }
    SDL_Renderer* get_renderer() const { return renderer; }
    
    // Event handling and rendering
    bool handle_events(std::atomic<bool>& running, std::atomic<bool>& paused);
    void render_frame();
    void limit_frame_rate(int target_fps);
    
    // Main thread rendering loop
    void render_loop(std::atomic<bool>& running, std::atomic<bool>& paused);
}; 