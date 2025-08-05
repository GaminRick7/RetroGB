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
#include "dma.hpp"
#include "lcd.hpp"
#include "joypad.hpp"
#include <thread>
#include <mutex>
#include <atomic>

/**
 * @brief Emulator context for thread-safe state management
 * 
 * Contains atomic variables for thread-safe communication between
 * the CPU thread and the main UI thread.
 */
class EmuContext {
public:
    std::atomic<bool> paused;   // Pause emulation
    std::atomic<bool> running;  // Continue emulation
    std::atomic<bool> die;      // Signal to stop emulation
    std::atomic<u64> ticks;     // Total CPU ticks executed
    std::mutex context_mutex;   // Mutex for complex operations
};

/**
 * @brief Main Game Boy emulator class
 * 
 * Coordinates all emulator components including:
 * - CPU execution in separate thread
 * - UI rendering and event handling
 * - Memory management and bus routing
 * - Component initialization and cleanup
 */
class Emulator {
public:
    // ===== CONSTRUCTORS & DESTRUCTORS =====
    Emulator();
    ~Emulator();
    
    // ===== MAIN EXECUTION =====
    int run(int argc, char** argv);
    
    // ===== CONTEXT ACCESS =====
    EmuContext* get_context();
    
    // ===== THREAD-SAFE STATE CONTROL =====
    void set_running(bool value) { ctx.running = value; }
    void set_paused(bool value) { ctx.paused = value; }
    void set_die(bool value) { ctx.die = value; }
    bool is_running() const { return ctx.running; }
    bool is_paused() const { return ctx.paused; }
    bool should_die() const { return ctx.die; }

private:
    // ===== EMULATOR CONTEXT =====
    EmuContext ctx;
    
    // ===== EMULATOR COMPONENTS =====
    Cartridge cartridge;
    RAM ram;
    Bus bus;
    CPU cpu;
    UI ui;
    IO io;
    Timer timer;
    PPU ppu;
    DMA dma;
    LCD lcd;
    Joypad joypad;
    
    // ===== THREADING =====
    std::thread cpu_thread;
    
    // ===== PRIVATE METHODS =====
    void cpu_run();  // CPU thread function
}; 