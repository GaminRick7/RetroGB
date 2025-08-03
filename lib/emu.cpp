#include "emu.hpp"
#include <chrono>
#include <thread>

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
    timer.set_cpu(&cpu);
    bus.set_ppu(&ppu);
    // Initialize UI
    if (!ui.init()) {
        printf("Failed to initialize UI\n");
        return -3;
    }

    printf("SDL window created successfully\n");

    // Start CPU thread
    try {
        cpu_thread = std::thread(&Emulator::cpu_run, this);
    } catch (const std::exception& e) {
        printf("Failed to start CPU thread: %s\n", e.what());
        return -4;
    }

    printf("CPU thread created successfully\n");

    // Wait for CPU thread to start and set running flag
    while (!ctx.running && !ctx.die) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Main thread handles UI rendering and events
    ui.render_loop(ctx.running, ctx.paused);
    
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