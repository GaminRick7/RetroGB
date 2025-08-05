#include "cpu.hpp"
#include <cstdio>

// ===== REGISTER OPERATIONS =====

u16 CPU::cpu_read_reg(RegType reg) {
    switch (reg) {
        case RegType::A:
            return regs.a;
        case RegType::B:
            return regs.b;
        case RegType::C:
            return regs.c;
        case RegType::D:
            return regs.d;
        case RegType::E:
            return regs.e;
        case RegType::H:
            return regs.h;
        case RegType::L:
            return regs.l;
        case RegType::AF:
            return (regs.a << 8) | regs.f;
        case RegType::BC:
            return (regs.b << 8) | regs.c;
        case RegType::DE:
            return (regs.d<< 8) | regs.e;
        case RegType::HL:
            return (regs.h << 8) | regs.l;
        case RegType::SP:
            return regs.sp;
        case RegType::PC:
            return regs.pc;
        case RegType::NONE:
        default:
            printf("CPU: Attempted to read from invalid register!\n");
            return 0;
    }
}

void CPU::cpu_set_reg(RegType reg, u16 value) {
    switch (reg) {
        case RegType::A:
            regs.a = value & 0xFF;
            break;
        case RegType::B:
            regs.b = value & 0xFF;
            break;
        case RegType::C:
            regs.c = value & 0xFF;
            break;
        case RegType::D:
            regs.d = value & 0xFF;
            break;
        case RegType::E:
            regs.e = value & 0xFF;
            break;
        case RegType::H:
            regs.h = value & 0xFF;
            break;
        case RegType::L:
            regs.l = value & 0xFF;
            break;
        case RegType::F:
            regs.f = value & 0xFF;
            break;
        case RegType::AF:
            regs.a = (value >> 8) & 0xFF;
            regs.f = value & 0xF0;  // Clear lower nibble - flags should always be zero
            break;
        case RegType::BC:
            regs.b = (value >> 8) & 0xFF;
            regs.c = value & 0xFF;
            break;
        case RegType::DE:
            regs.d = (value >> 8) & 0xFF;
            regs.e = value & 0xFF;
            break;
        case RegType::HL:
            regs.h = (value >> 8) & 0xFF;
            regs.l = value & 0xFF;
            break;
        case RegType::SP:
            regs.sp = value;
            break;
        case RegType::PC:
            regs.pc = value;
            break;
        case RegType::NONE:
        default:
            printf("CPU: Attempted to write to invalid register!\n");
            break;
    }
}

// ===== FLAG OPERATIONS =====

void CPU::set_flags(u8 z, u8 n, u8 h, u8 c) {
    if (z != 0xFF) set_flag(FLAG_Z, z);
    if (n != 0xFF) set_flag(FLAG_N, n);
    if (h != 0xFF) set_flag(FLAG_H, h);
    if (c != 0xFF) set_flag(FLAG_C, c);
}

void CPU::set_flag(u8 flag, bool value) {
    if (value)
        regs.f |= flag;
    
    else
        regs.f &= ~flag;
}

bool CPU::get_flag(u8 flag) const {
    return (regs.f & flag) != 0;
}

// ===== UTILITY FUNCTIONS =====

u16 CPU::little_to_big_endian(u16 little_endian) {
    return ((little_endian & 0xFF) << 8) | ((little_endian >> 8) & 0xFF);
} 