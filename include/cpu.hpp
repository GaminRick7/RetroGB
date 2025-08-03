#pragma once

#include "common.hpp"
#include "instructions.hpp"
#include "bus.hpp"
#include "timer.hpp"


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
    Timer* timer;

    u16 mem_dest;
    bool dest_is_mem;
    u8 cur_opcode;
    Instruction* curr_inst;
    int ticks;

    bool halted;
    bool stopped;
    bool ime; // Interrupt Master Enable flag
    bool enabling_ime;
    CPU();
    ~CPU();
    
    void init();
    void set_bus(Bus* b) { bus = b; }
    void set_timer(Timer* t) { timer = t; }
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
    u8 int_flags;

    // Getter and setter for IE register
    u8 get_ie_register() const;
    void set_ie_register(u8 value);

    // Stack operations
    void stack_push(u8 value);
    void stack_push16(u16 value);
    u8 stack_pop();
    u16 stack_pop16();
    u16 stack_peek();

    // Interrupt handling
    void set_int_flags(u8 flags);
    u8 get_int_flags();
    void clear_int_flags();
    void request_interrupt(u8 interrupt_type);
    void int_handle(u16 address);
    bool int_check(u16 address, u8 interrupt_type);
    void handle_interrupts();

    char dbg_msg[1024]= {0};
    int msg_size=0;

    void dbg_update();
    void dbg_print();

private:
    u8 ie_register;
};
