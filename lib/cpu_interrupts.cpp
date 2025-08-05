#include "cpu.hpp"
#include <cstdio>

// ===== INTERRUPT HANDLING =====

void CPU::set_int_flags(u8 flags) {
    int_flags = flags;
}

u8 CPU::get_int_flags() {
    return int_flags;
}

void CPU::clear_int_flags() {
    int_flags = 0;
}

void CPU::request_interrupt(u8 interrupt_type) {
    // Set the corresponding interrupt flag
    int_flags |= interrupt_type;
}

void CPU::int_handle(u16 address) {
    stack_push16(regs.pc);
    regs.pc = address;
}

bool CPU::int_check(u16 address, u8 interrupt_type) {
    if (int_flags & interrupt_type && ie_register & interrupt_type) {
        int_handle(address);
        int_flags &= ~interrupt_type;
        halted = false;
        ime = false;
        
        // Consume cycles for interrupt handling
        emu_cycles(5);
        
        return true;
    }
    
    return false;
}

void CPU::handle_interrupts() {
    if (int_check(0x40, IT_VBLANK)) {
        
    } else if (int_check(0x48, IT_LCD_STAT)) {
        
    } else if (int_check(0x50, IT_TIMER)) {
        
    } else if (int_check(0x58, IT_SERIAL)) {
        
    } else if (int_check(0x60, IT_JOYPAD)) {
        
    }
}

// ===== INTERRUPT ENABLE REGISTER =====

u8 CPU::get_ie_register() const {
    return ie_register;
}

void CPU::set_ie_register(u8 value) {
    ie_register = value;
} 