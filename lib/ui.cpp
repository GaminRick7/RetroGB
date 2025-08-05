#include "ui.hpp"
#include "ppu.hpp"
#include <SDL.h>
#include <SDL_ttf.h>

constexpr int SCREEN_WIDTH = 160 * 4;
constexpr int SCREEN_HEIGHT = 144 * 4;

UI::UI() : initialized(false), debug_enabled(DEBUG_MODE), window(nullptr), renderer(nullptr), debug_window(nullptr), debug_renderer(nullptr), debug_texture(nullptr), scale(4), bus(nullptr) {
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
        SCREEN_WIDTH,
        SCREEN_HEIGHT, 
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
    
    if (debug_enabled) {
        if (SDL_CreateWindowAndRenderer(16 * 8 * scale, 32 * 8 * scale, 0, 
            &debug_window, &debug_renderer) < 0) {
            printf("Debug window and renderer could not be created! SDL_Error: %s\n", SDL_GetError());
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        debug_texture = SDL_CreateTexture(debug_renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            (16 * 8 * scale) + (16 * scale), 
            (32 * 8 * scale) + (64 * scale));

        debug_screen = SDL_CreateRGBSurface(0, (16 * 8 * scale) + (16 * scale), 
            (32 * 8 * scale) + (64 * scale), 32,
            0x00FF0000,
            0x0000FF00,
            0x000000FF,
            0xFF000000);
    }

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Set window position
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    if (debug_enabled) {
        SDL_SetWindowPosition(debug_window, x + 160 * 4 + 10, y);
    }

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
        if (debug_enabled) {
            if (debug_renderer) {
                SDL_DestroyRenderer(debug_renderer);
                debug_renderer = nullptr;
            }
            if (debug_window) {
                SDL_DestroyWindow(debug_window);
                debug_window = nullptr;
            }
            if (debug_texture) {
                SDL_DestroyTexture(debug_texture);
                debug_texture = nullptr;
            }
            if (debug_screen) {
                SDL_FreeSurface(debug_screen);
                debug_screen = nullptr;
            }
        }
        TTF_Quit();
        SDL_Quit();
        initialized = false;
        printf("UI cleaned up\n");
    }
}

void UI::display_tile(SDL_Surface* surface, u16 addr, u16 tileNum, int x, int y) {
    SDL_Rect rc;

    for (int tileY=0; tileY<16; tileY+=2) {
        u8 b1 = bus->read(addr + tileNum*16 + tileY);
        u8 b2 = bus->read(addr + tileNum*16 + tileY + 1);

        for (int bit= 7; bit>=0; bit--) {
            u8 hi = !!(b1 & (1 << bit)) << 1;
            u8 lo = !!(b2 & (1 << bit));

            u8 color = hi | lo;

            rc.x = x + ((7 - bit) * scale);
            rc.y = y + (tileY / 2 * scale);
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(surface, &rc, tile_colors[color]);
        }
    }
}

void UI::update_debug_window() {
    if (!debug_enabled) {
        return;
    }
    
    int xDraw = 0;
    int yDraw = 0;
    int tileNum = 0;
    const u16 tile_data_addr = 0x8000; // Game Boy tile data address

    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = debug_screen->w;
    rc.h = debug_screen->h;
    SDL_FillRect(debug_screen, &rc, 0xFF111111);

    //384 tiles, 24 x 16
    for (int y=0; y<24; y++) {
        for (int x=0; x<16; x++) {
            display_tile(debug_screen, tile_data_addr, tileNum, xDraw + (x * scale), yDraw + (y * scale));
            xDraw += (8 * scale);
            tileNum++;
        }

        yDraw += (8 * scale);
        xDraw = 0;
    }

    SDL_UpdateTexture(debug_texture, NULL, debug_screen->pixels, debug_screen->pitch);
	SDL_RenderClear(debug_renderer);
	SDL_RenderCopy(debug_renderer, debug_texture, NULL, NULL);
	SDL_RenderPresent(debug_renderer);
}
void UI::delay(u32 ms) {
    SDL_Delay(ms);
}

void delay(u32 ms) {
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
                    case SDLK_p:
                        paused = !paused;
                        printf("Pause toggled: %s\n", paused ? "Paused" : "Running");
                        break;
                    default:
                        // Handle joypad input
                        if (joypad) {
                            joypad->update_from_sdl_key(event.key.keysym.sym, true);
                        }
                        break;
                }
                break;
            case SDL_KEYUP:
                // Handle joypad input release
                if (joypad) {
                    joypad->update_from_sdl_key(event.key.keysym.sym, false);
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

void UI::update() {
    SDL_Rect rc;
    rc.x = 0;
    rc.y = 0;
    rc.w = 2048;
    rc.h = 2048;

    u32* video_buffer = ppu->video_buffer;

    for (int line_num = 0; line_num < YRES; line_num++) {
        for (int x=0; x<XRES; x++) {
            rc.x = x * scale;
            rc.y = line_num * scale;
            rc.w = scale;
            rc.h = scale;
            
            u32 color = video_buffer[line_num * XRES + x];
            SDL_FillRect(screen, &rc, color);
        }
    }

    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    update_debug_window();
} 