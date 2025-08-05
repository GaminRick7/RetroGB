#include "emu.hpp"
#include <chrono>
#include <thread>
#include <SDL.h>

Emulator::Emulator() {
    ctx.paused = false;
    ctx.running = false;
    ctx.die = false;
    ctx.ticks = 0;
}

Emulator::~Emulator() {
    // Signal CPU thread to stop
    ctx.die = true;
    ctx.running = false;
    
    // Wait for CPU thread to finish
    if (cpu_thread.joinable()) {
        cpu_thread.join();
    }
}

void Emulator::cpu_run() {
    printf("CPU thread started\n");
    
    // Initialize CPU
    cpu.init();
    cpu.set_bus(&bus);
    cpu.set_timer(&timer);
    
    ctx.running = true;
    ctx.paused = false;
    ctx.die = false;
    ctx.ticks = 0;

    while(ctx.running && !ctx.die) {
        if (ctx.paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (!cpu.step()) {
            printf("CPU Stopped\n");
            ctx.running = false;
            break;
        }
        
        ctx.ticks++;
    }
    
    printf("CPU thread finished\n");
}

int Emulator::run(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    if (!cartridge.load(argv[1])) {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("Cart loaded..\n");

    // Connect bus to cartridge
    bus.set_cartridge(&cartridge);
    bus.set_ram(&ram);
    bus.set_cpu(&cpu);
    bus.set_io(&io);
    io.set_timer(&timer);
    io.set_cpu(&cpu);
    io.set_joypad(&joypad);
    timer.set_cpu(&cpu);
    bus.set_ppu(&ppu);
    dma.set_ppu(&ppu);
    dma.set_bus(&bus);
    io.set_dma(&dma);
    cpu.set_dma(&dma);
    cpu.set_ppu(&ppu);
    bus.set_dma(&dma);
    io.set_lcd(&lcd);
    lcd.set_dma(&dma);
    ppu.set_lcd(&lcd);
    ppu.set_cpu(&cpu);
    ppu.set_bus(&bus);
    ppu.set_cart(&cartridge);
    // Set bus reference for UI
    ui.set_bus(&bus);
    ui.set_ppu(&ppu);
    ui.set_joypad(&joypad);
    // Initialize UI
    if (!ui.init()) {
        printf("Failed to initialize UI\n");
        return -3;
    }

    ppu.init();

    printf("SDL window created successfully\n");

    // Start CPU thread
    try {
        cpu_thread = std::thread(&Emulator::cpu_run, this);
    } catch (const std::exception& e) {
        printf("Failed to start CPU thread: %s\n", e.what());
        return -4;
    }

    printf("CPU thread created successfully\n");

    u32 prev_frame = 0;
    const int target_fps = 60;
    const int frame_delay = 1000 / target_fps;
    u32 last_frame_time = 0;

    // Main loop - handle events and rendering
    while (!ctx.die) {
        u32 frame_start = SDL_GetTicks();
        
        // Handle events
        if (!ui.handle_events(ctx.running, ctx.paused)) {
            ctx.die = true;
            break;
        }
        
        // Update display if frame has changed
        if (prev_frame != ppu.current_frame) {
            ui.update();
            prev_frame = ppu.current_frame;
        }
        
        // Frame rate limiting
        u32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frame_delay) {
            SDL_Delay(frame_delay - frame_time);
        }
    }
    
    // Signal CPU thread to stop
    ctx.die = true;

    // Wait for CPU thread to finish
    if (cpu_thread.joinable()) {
        cpu_thread.join();
    }

    printf("Emulator stopped\n");
    return 0;
}

EmuContext* Emulator::get_context() {
    return &ctx;
} 