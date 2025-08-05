#pragma once

#include "common.hpp"
#include "instructions.hpp"
#include "bus.hpp"
#include "timer.hpp"
#include "dma.hpp"

class Bus;
class Timer;
class DMA;
class PPU;

// Forward declaration for instruction processor function type
using InstrFunc = void (*)(CPU* cpu, const Instruction* inst);

// Function declaration for instruction processor lookup
InstrFunc inst_get_processor(InType type);

/**
 * @brief CPU registers structure
 * 
 * Contains all Game Boy CPU registers including:
 * - 8-bit registers: A, F, B, C, D, E, H, L
 * - 16-bit registers: PC (Program Counter), SP (Stack Pointer)
 * - F register contains flags: Z, N, H, C
 */
class Registers {
public:
    u16 pc;  // Program Counter
    u16 sp;  // Stack Pointer
    u8 a;    // Accumulator
    u8 f;    // Flags register
    u8 b;    // General purpose register
    u8 c;    // General purpose register
    u8 d;    // General purpose register
    u8 e;    // General purpose register
    u8 h;    // General purpose register
    u8 l;    // General purpose register
};

/**
 * @brief Game Boy CPU emulator
 * 
 * Implements the Z80-like CPU used in the Game Boy, including:
 * - All standard instructions
 * - Interrupt handling
 * - Memory management
 * - Cycle-accurate timing
 */
class CPU {
public:
    // ===== CONSTRUCTORS & DESTRUCTORS =====
    CPU();
    ~CPU();
    
    // ===== INITIALIZATION =====
    void init();
    
    // ===== MAIN EXECUTION =====
    bool step();
    
    // ===== COMPONENT CONNECTIONS =====
    void set_bus(Bus* b) { bus = b; }
    void set_timer(Timer* t) { timer = t; }
    void set_dma(DMA* d) { dma = d; }
    void set_ppu(PPU* p) { ppu = p; }
    
    // ===== REGISTER OPERATIONS =====
    u16 cpu_read_reg(RegType reg);
    void cpu_set_reg(RegType reg, u16 value);
    
    // ===== FLAG OPERATIONS =====
    void set_flags(u8 z, u8 n, u8 h, u8 c);
    void set_flag(u8 flag, bool value);
    bool get_flag(u8 flag) const;
    
    // ===== STACK OPERATIONS =====
    void stack_push(u8 value);
    void stack_push16(u16 value);
    u8 stack_pop();
    u16 stack_pop16();
    u16 stack_peek();
    
    // ===== INTERRUPT HANDLING =====
    void set_int_flags(u8 flags);
    u8 get_int_flags();
    void clear_int_flags();
    void request_interrupt(u8 interrupt_type);
    void int_handle(u16 address);
    bool int_check(u16 address, u8 interrupt_type);
    void handle_interrupts();
    
    // ===== INTERRUPT ENABLE REGISTER =====
    u8 get_ie_register() const;
    void set_ie_register(u8 value);
    
    // ===== UTILITY FUNCTIONS =====
    u16 little_to_big_endian(u16 little_endian);
    
    // ===== DEBUG FUNCTIONS =====
    void dbg_update();
    void dbg_print();

    // ===== PUBLIC MEMBERS FOR INSTRUCTION EXECUTION =====
    Registers regs;
    Bus* bus;
    u16 fetched_data;
    u16 mem_dest;
    bool dest_is_mem;
    void emu_cycles(int cycles);
    bool ime;              // Interrupt Master Enable flag
    bool enabling_ime;
    u8 cur_opcode;
    bool halted;

private:
    // ===== COMPONENT REFERENCES =====
    Timer* timer;
    DMA* dma;
    PPU* ppu;
    
    // ===== CPU STATE =====
    bool stopped;
    int ticks;
    
    // ===== INSTRUCTION EXECUTION STATE =====
    Instruction* curr_inst;
    u8 int_flags;
    u8 ie_register;
    
    // ===== INSTRUCTION FETCHING =====
    void fetch_instruction();
    void fetch_data();
    void execute();
    
    // ===== DEBUG STATE =====
    char dbg_msg[1024];
    int msg_size;
};
