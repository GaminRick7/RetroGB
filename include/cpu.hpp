#pragma once

#include "common.hpp"
#include "instructions.hpp"
#include "bus.hpp"


constexpr uint8_t FLAG_Z = 1 << 7; // Zero
constexpr uint8_t FLAG_N = 1 << 6; // Subtract
constexpr uint8_t FLAG_H = 1 << 5; // Half-Carry
constexpr uint8_t FLAG_C = 1 << 4; // Carry

class Registers {
public:
    u16 pc;
    u16 sp;
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
};

class CPU {
public:

    Bus* bus;

    u16 mem_dest;
    bool dest_is_mem;
    u8 cur_opcode;
    Instruction* curr_inst;

    bool halted;
    bool stopped;
    bool ime; // Interrupt Master Enable flag
    CPU();
    ~CPU();
    
    void init();
    void set_bus(Bus* b) { bus = b; }
    bool step();
    
    // Instruction fetching
    void fetch_instruction();
    void fetch_data();
    void execute();
    
    // Cycle management
    void emu_cycles(int cycles);
    
    // Register operations
    u16 cpu_read_reg(RegType reg);
    void cpu_set_reg(RegType reg, u16 value);
    
    // Utility functions
    u16 little_to_big_endian(u16 little_endian);

    // Flag helpers
    void set_flags(u8 z, u8 n, u8 h, u8 c);
    void set_flag(u8 flag, bool value);
    bool get_flag(u8 flag) const;


    u16 fetched_data;
    Registers regs;

    // Getter and setter for IE register
    u8 get_ie_register() const;
    void set_ie_register(u8 value);

    // Stack operations
    void stack_push(u8 value);
    void stack_push16(u16 value);
    u16 stack_pop();
    u8 stack_pop16();
    u16 stack_peek();

private:
    u8 ie_register;
};
