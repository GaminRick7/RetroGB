#include "cpu.hpp"
#include <cstdio>

// ===== STACK OPERATIONS =====

void CPU::stack_push(u8 value) {
    regs.sp--;
    bus->write(regs.sp, value);
}

void CPU::stack_push16(u16 value) {
    stack_push((value >> 8) & 0xFF);
    stack_push(value & 0xFF);
}

u8 CPU::stack_pop() {
    return bus->read(regs.sp++);
}

u16 CPU::stack_pop16() {
    u8 lo = stack_pop();
    u8 hi = stack_pop();
    return (hi << 8) | lo;  // Combine high and low bytes
}

u16 CPU::stack_peek() {
    u8 lo = bus->read(regs.sp);
    u8 hi = bus->read(regs.sp + 1);
    return (hi << 8) | lo;
} 