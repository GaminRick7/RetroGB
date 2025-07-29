#include "cpu.hpp"
#include <unordered_map>
#include <cstdio>
#include <cstdlib>

using InstrFunc = void (*)(CPU* cpu, const Instruction* inst);

// Helper function to check if register is 16-bit
static bool is_16_bit(RegType reg) {
    return (reg == RegType::SP || reg == RegType::BC || 
            reg == RegType::DE || reg == RegType::HL);
}

// Generic condition checker
static bool check_condition(CPU* cpu, CondType cond) {
    switch (cond) {
        case CondType::Z:   return cpu->get_flag(FLAG_Z);
        case CondType::NZ:  return !cpu->get_flag(FLAG_Z);
        case CondType::C:   return cpu->get_flag(FLAG_C);
        case CondType::NC:  return !cpu->get_flag(FLAG_C);
        case CondType::NONE:
            default:            return true;
    }
}

// All processor functions should match this signature
static void proc_none(CPU* cpu, const Instruction* inst) {
    printf("INVALID INSTRUCTION!\n");
    exit(-7);
}

static void goto_addr(CPU* cpu, u16 addr, bool push_pc) {
    if (push_pc) {
        cpu->stack_push16(cpu->regs.pc);
        cpu->emu_cycles(2);
    }
    cpu->regs.pc = addr;
    cpu->emu_cycles(1);
}

static void proc_nop(CPU* cpu, const Instruction* inst) {
    // NOP: No Operation
    cpu->emu_cycles(1);
}

static void proc_ld(CPU* cpu, const Instruction* inst) {
    // Special case: HL = SP + e8 (HL_SPR)
    if (inst->mode == AddrMode::HL_SPR) {
        u16 result = cpu->fetched_data;
        cpu->cpu_set_reg(RegType::HL, result);
        // Set flags: Z=0, N=0, H, C according to GB rules
        u8 sp = cpu->regs.sp;
        int8_t n = static_cast<int8_t>(cpu->bus->read(cpu->regs.pc - 1)); // last fetched immediate
        u8 half_carry = ((sp & 0xF) + (n & 0xF)) > 0xF;
        u8 carry = ((sp & 0xFF) + (n & 0xFF)) > 0xFF;
        cpu->set_flags(0, 0, half_carry, carry);
        return;
    }
    // Special case: 16-bit bus write (e.g., LD (a16), SP)
    if (cpu->dest_is_mem && (inst->reg_2 == RegType::SP || inst->reg_2 == RegType::HL || inst->reg_2 == RegType::DE || inst->reg_2 == RegType::BC)) {
        cpu->bus->write16(cpu->mem_dest, cpu->fetched_data);
        return;
    }
    // Standard cases
    if (cpu->dest_is_mem) {
        cpu->bus->write(cpu->mem_dest, cpu->fetched_data);
    } else {
        cpu->cpu_set_reg(inst->reg_1, cpu->fetched_data);
    }
    cpu->emu_cycles(1);
}

static void proc_ldh(CPU* cpu, const Instruction* inst) {
    if (cpu->dest_is_mem) {
        cpu->bus->write(cpu->mem_dest, cpu->regs.a);
    } else {
        cpu->cpu_set_reg(RegType::A, cpu->bus->read(0xFF00 | cpu->fetched_data));
    }
    cpu->emu_cycles(1);
}

static void proc_jp(CPU* cpu, const Instruction* inst) {
    goto_addr(cpu, cpu->fetched_data, false);
}

static void proc_call(CPU* cpu, const Instruction* inst) {
    goto_addr(cpu, cpu->fetched_data, true);
}

static void proc_ret(CPU* cpu, const Instruction* inst) {
    // RET: 4 cycles (unconditional) or 5 cycles (conditional, if condition met)
    // Base cycle for all RET instructions
    cpu->emu_cycles(1);
    
    // For conditional RET, add 1 extra cycle if condition is met
    if (inst->cond != CondType::NONE) {
        cpu->emu_cycles(1);
    }

    if (check_condition(cpu, inst->cond)) {
        u16 lo = cpu->stack_pop();
        cpu->emu_cycles(1);
        u16 hi = cpu->stack_pop();
        cpu->emu_cycles(1);

        u16 n = (hi << 8) | lo;
        cpu->regs.pc = n;

        cpu->emu_cycles(1);
    }
}

static void proc_reti(CPU* cpu, const Instruction* inst) {
    // Use the same logic as RET (but with NONE condition to always execute)
    proc_ret(cpu, inst);
    
    // Re-enable interrupts (the key difference from RET)
    cpu->ime = true;
}

static void proc_rst(CPU* cpu, const Instruction* inst) {
    // Calculate target address based on opcode
    u8 opcode = cpu->cur_opcode;
    u16 target_addr;
    
    switch (opcode) {
        case 0xC7: target_addr = 0x0000; break;  // RST 00H
        case 0xCF: target_addr = 0x0008; break;  // RST 08H
        case 0xD7: target_addr = 0x0010; break;  // RST 10H
        case 0xDF: target_addr = 0x0018; break;  // RST 18H
        case 0xE7: target_addr = 0x0020; break;  // RST 20H
        case 0xEF: target_addr = 0x0028; break;  // RST 28H
        case 0xF7: target_addr = 0x0030; break;  // RST 30H
        case 0xFF: target_addr = 0x0038; break;  // RST 38H
        default: target_addr = 0x0000; break;
    }
    
    // Push return address and jump (same as CALL but to fixed address)
    goto_addr(cpu, target_addr, true);
}

static void proc_inc(CPU* cpu, const Instruction* inst) {
    if (inst->reg_1 == RegType::HL && inst->mode == AddrMode::MR) {
        // This is INC (HL) - memory operation
        u16 addr = cpu->cpu_read_reg(RegType::HL);
        u8 value = cpu->bus->read(addr);
        u8 result = value + 1;
        
        // Set flags: Z=result==0, N=0, H=half-carry
        bool half_carry = (value & 0x0F) == 0x0F;
        cpu->set_flags(result == 0, 0, half_carry, 0);
        
        cpu->bus->write(addr, result);
        cpu->emu_cycles(3); // Memory read + write + increment
    } else {
        // This is INC r - register operation
        u16 value = cpu->cpu_read_reg(inst->reg_1);
        
        if (inst->reg_1 == RegType::SP || inst->reg_1 == RegType::BC || 
            inst->reg_1 == RegType::DE || inst->reg_1 == RegType::HL) {
            // 16-bit increment
            u16 result = value + 1;
            cpu->cpu_set_reg(inst->reg_1, result);
            cpu->emu_cycles(2);
        } else {
            // 8-bit increment
            u8 result = (value & 0xFF) + 1;
            
            // Set flags: Z=result==0, N=0, H=half-carry
            bool half_carry = (value & 0x0F) == 0x0F;
            cpu->set_flags(result == 0, 0, half_carry, 0);
            
            cpu->cpu_set_reg(inst->reg_1, result);
            cpu->emu_cycles(1);
        }
    }
}

static void proc_dec(CPU* cpu, const Instruction* inst) {
    if (inst->reg_1 == RegType::HL && inst->mode == AddrMode::MR) {
        // This is DEC (HL) - memory operation
        u16 addr = cpu->cpu_read_reg(RegType::HL);
        u8 value = cpu->bus->read(addr);
        u8 result = value - 1;
        
        // Set flags: Z=result==0, N=1, H=half-borrow
        bool half_borrow = (value & 0x0F) == 0x00;
        cpu->set_flags(result == 0, 1, half_borrow, 0);
        
        cpu->bus->write(addr, result);
        cpu->emu_cycles(3); // Memory read + write + decrement
    } else {
        // This is DEC r - register operation
        u16 value = cpu->cpu_read_reg(inst->reg_1);
        
        if (inst->reg_1 == RegType::SP || inst->reg_1 == RegType::BC || 
            inst->reg_1 == RegType::DE || inst->reg_1 == RegType::HL) {
            // 16-bit decrement
            u16 result = value - 1;
            cpu->cpu_set_reg(inst->reg_1, result);
            cpu->emu_cycles(2);
        } else {
            // 8-bit decrement
            u8 result = (value & 0xFF) - 1;
            
            // Set flags: Z=result==0, N=1, H=half-borrow
            bool half_borrow = (value & 0x0F) == 0x00;
            cpu->set_flags(result == 0, 1, half_borrow, 0);
            
            cpu->cpu_set_reg(inst->reg_1, result);
            cpu->emu_cycles(1);
        }
    }
}

static void proc_sub(CPU* cpu, const Instruction* inst) {
    u16 val = cpu->cpu_read_reg(inst->reg_1) - cpu->fetched_data;

    int z = val == 0;
    int h = ((int)cpu->cpu_read_reg(inst->reg_1) & 0xF) - ((int)cpu->fetched_data & 0xF) < 0;
    int c = ((int)cpu->cpu_read_reg(inst->reg_1)) - ((int)cpu->fetched_data) < 0;

    cpu->cpu_set_reg(inst->reg_1, val);
    cpu->set_flags(z, 1, h, c);
}

static void proc_sbc(CPU* cpu, const Instruction* inst) {
    u8 val = cpu->fetched_data + cpu->get_flag(FLAG_C);

    int z = cpu->cpu_read_reg(inst->reg_1) - val == 0;

    int h = ((int)cpu->cpu_read_reg(inst->reg_1) & 0xF) 
        - ((int)cpu->fetched_data & 0xF) - ((int)cpu->get_flag(FLAG_C)) < 0;
    int c = ((int)cpu->cpu_read_reg(inst->reg_1)) 
        - ((int)cpu->fetched_data) - ((int)cpu->get_flag(FLAG_C)) < 0;

    cpu->cpu_set_reg(inst->reg_1, cpu->cpu_read_reg(inst->reg_1) - val);
    cpu->set_flags(z, 1, h, c);
}

static void proc_adc(CPU* cpu, const Instruction* inst) {
    u16 u = cpu->fetched_data;
    u16 a = cpu->regs.a;
    u16 c = cpu->get_flag(FLAG_C);

    cpu->regs.a = (a + u + c) & 0xFF;

    cpu->set_flags(cpu->regs.a == 0, 0, 
        (a & 0xF) + (u & 0xF) + c > 0xF,
        a + u + c > 0xFF);
}

static void proc_add(CPU* cpu, const Instruction* inst) {
    u32 val = cpu->cpu_read_reg(inst->reg_1) + cpu->fetched_data;

    bool is_16bit = is_16_bit(inst->reg_1);

    if (is_16bit) {
        cpu->emu_cycles(1);
    }

    if (inst->reg_1 == RegType::SP) {
        val = cpu->cpu_read_reg(inst->reg_1) + (char)cpu->fetched_data;
    }

    int z = (val & 0xFF) == 0;
    int h = (cpu->cpu_read_reg(inst->reg_1) & 0xF) + (cpu->fetched_data & 0xF) >= 0x10;
    int c = (int)(cpu->cpu_read_reg(inst->reg_1) & 0xFF) + (int)(cpu->fetched_data & 0xFF) >= 0x100;

    if (is_16bit) {
        z = -1;
        h = (cpu->cpu_read_reg(inst->reg_1) & 0xFFF) + (cpu->fetched_data & 0xFFF) >= 0x1000;
        u32 n = ((u32)cpu->cpu_read_reg(inst->reg_1)) + ((u32)cpu->fetched_data);
        c = n >= 0x10000;
    }

    if (inst->reg_1 == RegType::SP) {
        z = 0;
        h = (cpu->cpu_read_reg(inst->reg_1) & 0xF) + (cpu->fetched_data & 0xF) >= 0x10;
        c = (int)(cpu->cpu_read_reg(inst->reg_1) & 0xFF) + (int)(cpu->fetched_data & 0xFF) > 0x100;
    }

    cpu->cpu_set_reg(inst->reg_1, val & 0xFFFF);
    cpu->set_flags(z, 0, h, c);
}


static void proc_jr(CPU* cpu, const Instruction* inst) {
    char rel = (char)(cpu->fetched_data & 0xFF);
    u16 addr = cpu->regs.pc + rel;
    goto_addr(cpu, addr, false);
}

static void proc_di(CPU* cpu, const Instruction* inst) {
    cpu->ime = false;
}

static void proc_xor(CPU* cpu, const Instruction* inst) {
    // & 0xFF to ensure only lower 8 bits are used
    cpu->regs.a ^= cpu->fetched_data & 0xFF;
    cpu->set_flags(cpu->regs.a, 0, 0, 0);
    cpu->emu_cycles(1);
}

static void proc_push(CPU* cpu, const Instruction* inst) {
    u16 hi = (cpu->cpu_read_reg(inst->reg_1) >> 8) & 0xFF;
    cpu->emu_cycles(1);
    cpu->stack_push(hi);

    u16 lo = cpu->cpu_read_reg(inst->reg_1) & 0xFF;
    cpu->emu_cycles(1);
    cpu->stack_push(lo);
    
    cpu->emu_cycles(1);
}

static void proc_pop(CPU* cpu, const Instruction* inst) {
    u16 lo = cpu->stack_pop();
    cpu->emu_cycles(1);
    u16 hi = cpu->stack_pop();
    cpu->emu_cycles(1);
    u16 n = (hi << 8 ) | lo;
    cpu->cpu_set_reg(inst->reg_1, n);
    if (inst->reg_1 == RegType::AF) {
        cpu->cpu_set_reg(RegType::A, n & 0xFFF0);
    }
}



static std::unordered_map<InType, InstrFunc> processors = {
    {InType::NONE, proc_none},
    {InType::NOP,  proc_nop},
    {InType::LD,   proc_ld},
    {InType::LDH,  proc_ldh},
    {InType::JP,   proc_jp},
    {InType::DI,   proc_di},
    {InType::XOR,  proc_xor},
    {InType::PUSH, proc_push},
    {InType::POP,  proc_pop},
    {InType::CALL, proc_call},
    {InType::JR,   proc_jr},
    {InType::RET,  proc_ret},
    {InType::RETI, proc_reti},
    {InType::RST,  proc_rst},
    {InType::INC,  proc_inc},
    {InType::DEC,  proc_dec},
    {InType::ADD,  proc_add},
    {InType::ADC,  proc_adc},
    {InType::SUB,  proc_sub},
    {InType::SBC,  proc_sbc}
};

InstrFunc inst_get_processor(InType type) {
    auto it = processors.find(type);
    if (it != processors.end()) {
        return it->second;
    }
    return proc_none; // Default to error handler
}




