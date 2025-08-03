#pragma once

#include "common.hpp"
#include "cart.hpp"
#include "bus.hpp"
#include "cpu.hpp"
#include "ram.hpp"
#include "ui.hpp"
#include "io.hpp"
#include "timer.hpp"
#include "ppu.hpp"
#include <thread>
#include <mutex>
#include <atomic>

class EmuContext {
public:
    std::atomic<bool> paused;
    std::atomic<bool> running;
    std::atomic<bool> die;
    std::atomic<u64> ticks;
    std::mutex context_mutex;
};

class Emulator {
private:
    EmuContext ctx;
    Cartridge cartridge;
    RAM ram;
    Bus bus;
    CPU cpu;
    UI ui;
    IO io;
    Timer timer;
    PPU ppu;
    std::thread cpu_thread;
    
    // CPU thread function
    void cpu_run();

public:
    Emulator();
    ~Emulator();
    
    int run(int argc, char** argv);
    EmuContext* get_context();
    
    // Thread-safe context access
    void set_running(bool value) { ctx.running = value; }
    void set_paused(bool value) { ctx.paused = value; }
    void set_die(bool value) { ctx.die = value; }
    bool is_running() const { return ctx.running; }
    bool is_paused() const { return ctx.paused; }
    bool should_die() const { return ctx.die; }
}; 