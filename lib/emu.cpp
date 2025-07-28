#include "emu.hpp"
#include <SDL.h>
#include <SDL_ttf.h>

Emulator::Emulator() {
    ctx.paused = false;
    ctx.running = false;
    ctx.ticks = 0;
}

Emulator::~Emulator() {
    // Cleanup will be handled by RAII
}

void delay(u32 ms) {
    SDL_Delay(ms);
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

    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");

    // Initialize and connect CPU to bus
    cpu.init();
    cpu.set_bus(&bus);
    
    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while(ctx.running) {
        if (ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu.step()) {
            printf("CPU Stopped\n");
            return -3;
        }

        ctx.ticks++;
    }

    return 0;
}

EmuContext* Emulator::get_context() {
    return &ctx;
} 