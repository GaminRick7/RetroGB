#include "ui.hpp"
#include <SDL.h>
#include <SDL_ttf.h>

UI::UI() : initialized(false), window(nullptr), renderer(nullptr) {
}

UI::~UI() {
    cleanup();
}

bool UI::init() {
    if (initialized) {
        return true;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }
    
    // Create window and renderer
    if (SDL_CreateWindowAndRenderer(
        160 * 4,  // Game Boy screen width * 4 for scaling
        144 * 4,  // Game Boy screen height * 4 for scaling
        SDL_WINDOW_SHOWN,
        &window,
        &renderer
    ) < 0) {
        printf("Window and renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }
    
    // Set window title
    SDL_SetWindowTitle(window, "Game Boy Emulator");
    
    
    initialized = true;
    printf("UI initialized successfully with SDL window\n");
    return true;
}

void UI::cleanup() {
    if (initialized) {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        TTF_Quit();
        SDL_Quit();
        initialized = false;
        printf("UI cleaned up\n");
    }
}

void UI::delay(u32 ms) {
    SDL_Delay(ms);
}

bool UI::is_initialized() const {
    return initialized;
}

bool UI::handle_events(std::atomic<bool>& running, std::atomic<bool>& paused) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                printf("Window close requested\n");
                running = false;
                return false;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        printf("Escape key pressed\n");
                        running = false;
                        return false;
                    case SDLK_SPACE:
                        paused = !paused;
                        printf("Pause toggled: %s\n", paused ? "Paused" : "Running");
                        break;
                }
                break;
        }
    }
    return true;
}

void UI::render_frame() {
    if (!renderer) return;
    
    // Clear screen with Game Boy green color
    SDL_SetRenderDrawColor(renderer, 15, 56, 15, 255);
    SDL_RenderClear(renderer);
    
    // TODO: Add actual Game Boy screen rendering here
    // For now, just present the cleared screen
    SDL_RenderPresent(renderer);
}

void UI::limit_frame_rate(int target_fps) {
    static u32 last_frame_time = 0;
    const int frame_delay = 1000 / target_fps;
    
    u32 current_time = SDL_GetTicks();
    u32 frame_time = current_time - last_frame_time;
    
    if (frame_time < frame_delay) {
        SDL_Delay(frame_delay - frame_time);
    }
    
    last_frame_time = SDL_GetTicks();
}

void UI::render_loop(std::atomic<bool>& running, std::atomic<bool>& paused) {
    const int target_fps = 60;
    const int frame_delay = 1000 / target_fps;
    u32 last_frame_time = 0;
    int frame_count = 0;
    
    while (running) {
        u32 frame_start = SDL_GetTicks();
        
        // Handle events
        if (!handle_events(running, paused)) {
            break;
        }
        
        // Render frame
        render_frame();
        
        // Frame rate limiting
        u32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frame_delay) {
            SDL_Delay(frame_delay - frame_time);
        }
        
        frame_count++;
        if (frame_count % 60 == 0) {
            printf("UI FPS: %d\n", target_fps);
        }
    }
} 