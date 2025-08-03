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

static void goto_addr(CPU* cpu, u16 addr, bool push_pc, CondType cond) {
    if (check_condition(cpu, cond)) {
        if (push_pc) {
            cpu->stack_push16(cpu->regs.pc);
            cpu->emu_cycles(2);
        }
            cpu->regs.pc = addr;
            cpu->emu_cycles(1);
    }
}

static void proc_nop(CPU* cpu, const Instruction* inst) {
    // NOP: No Operation
    cpu->emu_cycles(1);
}

static void proc_ld(CPU* cpu, const Instruction* inst) {
    // Special case: HL = SP + e8 (HL_SPR)
    if (inst->mode == AddrMode::HL_SPR) {
        u16 sp = cpu->regs.sp;
        int8_t e8 = static_cast<int8_t>(cpu->fetched_data);
        u16 result = sp + e8;
        cpu->cpu_set_reg(RegType::HL, result);
        
        // Set flags: Z=0, N=0, H=half-carry, C=carry
        // Check half-carry (bit 3 to bit 4)
        bool half_carry = ((sp & 0xF) + (e8 & 0xF)) > 0xF;
        // Check carry (bit 7 to bit 8)
        bool carry = ((sp & 0xFF) + (e8 & 0xFF)) > 0xFF;
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
    goto_addr(cpu, cpu->fetched_data, false, inst->cond);
}

static void proc_call(CPU* cpu, const Instruction* inst) {
    goto_addr(cpu, cpu->fetched_data, true, inst->cond);
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
    goto_addr(cpu, target_addr, true, CondType::NONE);
}

static void proc_inc(CPU* cpu, const Instruction* inst) {
    u16 val = cpu->cpu_read_reg(inst->reg_1) + 1;

    if (is_16_bit(inst->reg_1)) {
        cpu->emu_cycles(1);
    }

    if (inst->reg_1 == RegType::HL && inst->mode == AddrMode::MR) {
        val = cpu->bus->read(cpu->cpu_read_reg(RegType::HL)) + 1;
        val &= 0xFF;
        cpu->bus->write(cpu->cpu_read_reg(RegType::HL), val);
    } else {
        cpu->cpu_set_reg(inst->reg_1, val);
        val = cpu->cpu_read_reg(inst->reg_1);
    }

    if ((cpu->cur_opcode & 0x03) == 0x03) {
        return;
    }

    cpu->set_flags(val == 0, 0, (val & 0x0F) == 0, -1);
}

static void proc_dec(CPU* cpu, const Instruction* inst) {
    if (inst->reg_1 == RegType::HL && inst->mode == AddrMode::MR) {
        // This is DEC (HL) - memory operation
        u16 addr = cpu->cpu_read_reg(RegType::HL);
        u8 value = cpu->bus->read(addr);
        u8 result = value - 1;
        
        // Set flags: Z=result==0, N=1, H=half-borrow, C=preserved
        bool half_borrow = (value & 0x0F) == 0x00;  // Half-borrow occurs when lower nibble was 0x00 before subtraction
        cpu->set_flags(result == 0, 1, half_borrow, cpu->get_flag(FLAG_C));
        
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
            
            // Set flags: Z=result==0, N=1, H=half-borrow, C=preserved
            bool half_borrow = (value & 0x0F) == 0x00;  // Half-borrow occurs when lower nibble was 0x00 before subtraction
            cpu->set_flags(result == 0, 1, half_borrow, cpu->get_flag(FLAG_C));
            
            cpu->cpu_set_reg(inst->reg_1, result);
            cpu->emu_cycles(1);
        }
    }
}

static void proc_sub(CPU* cpu, const Instruction* inst) {
    u8 a = cpu->cpu_read_reg(inst->reg_1) & 0xFF;
    u8 b = cpu->fetched_data & 0xFF;
    u16 val = a - b;

    // Z flag: result is zero
    int z = (val & 0xFF) == 0;
    
    // N flag: always 1 for subtraction
    int n = 1;
    
    // H flag: half-carry (borrow from bit 3 to bit 4)
    int h = (a & 0xF) < (b & 0xF);
    
    // C flag: carry (borrow from bit 7 to bit 8)
    int c = a < b;

    cpu->cpu_set_reg(inst->reg_1, val);
    cpu->set_flags(z, n, h, c);
}

static void proc_sbc(CPU* cpu, const Instruction* inst) {
    u8 a = cpu->cpu_read_reg(inst->reg_1) & 0xFF;
    u8 b = cpu->fetched_data & 0xFF;
    u8 carry_in = cpu->get_flag(FLAG_C);
    u16 val = a - b - carry_in;

    // Z flag: result is zero
    int z = (val & 0xFF) == 0;
    
    // N flag: always 1 for subtraction
    int n = 1;
    
    // H flag: half-carry (borrow from bit 3 to bit 4)
    int h = (a & 0xF) < ((b & 0xF) + carry_in);
    
    // C flag: carry (borrow from bit 7 to bit 8)
    int c = a < (b + carry_in);

    cpu->cpu_set_reg(inst->reg_1, val);
    cpu->set_flags(z, n, h, c);
}

// CB instruction processor - reads next byte and dispatches to appropriate bit manipulation instruction
static void proc_cb(CPU* cpu, const Instruction* inst) {
    u8 cb_opcode = cpu->fetched_data;
    // Decode the CB instruction
    u8 bit = (cb_opcode >> 3) & 0x07;  // Bits 5-3: bit number
    u8 reg = cb_opcode & 0x07;          // Bits 2-0: register
    u8 op = (cb_opcode >> 6) & 0x03;   // Bits 7-6: operation
    
    // Determine register type from reg field
    RegType reg_type;
    switch (reg) {
        case 0: reg_type = RegType::B; break;
        case 1: reg_type = RegType::C; break;
        case 2: reg_type = RegType::D; break;
        case 3: reg_type = RegType::E; break;
        case 4: reg_type = RegType::H; break;
        case 5: reg_type = RegType::L; break;
        case 6: reg_type = RegType::HL; break; // (HL) - memory
        case 7: reg_type = RegType::A; break;
        default: reg_type = RegType::A; break;
    }
    // Get the value to operate on
    u8 value;
    bool is_memory = (reg_type == RegType::HL);
    if (is_memory) {
        u16 addr = cpu->cpu_read_reg(RegType::HL);
        value = cpu->bus->read(addr);
        cpu->emu_cycles(1); // Memory read
    } else {
        value = cpu->cpu_read_reg(reg_type) & 0xFF;
    }
    
    u8 result = 0;
    bool set_z = false;
    bool set_n = false;
    bool set_h = false;
    bool set_c = false;
    
    // Execute the bit manipulation operation
    switch (op) {
        case 0: // RLC, RRC, RL, RR, SLA, SRA, SWAP, SRL
            switch (bit) {
                case 0: // RLC r
                    result = ((value << 1) | (value >> 7)) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = (value & 0x80) != 0;
                    break;
                case 1: // RRC r
                    result = ((value >> 1) | (value << 7)) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = (value & 0x01) != 0;
                    break;
                case 2: // RL r
                    result = ((value << 1) | cpu->get_flag(FLAG_C)) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = (value & 0x80) != 0;
                    break;
                case 3: // RR r
                    result = ((value >> 1) | (cpu->get_flag(FLAG_C) << 7)) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = (value & 0x01) != 0;
                    break;
                case 4: // SLA r
                    result = (value << 1) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = (value & 0x80) != 0;
                    break;
                case 5: // SRA r
                    result = ((value >> 1) | (value & 0x80)) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = (value & 0x01) != 0;
                    break;
                case 6: // SWAP r
                    result = ((value << 4) | (value >> 4)) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = false;
                    break;
                case 7: // SRL r
                    result = (value >> 1) & 0xFF;
                    set_z = (result == 0);
                    set_n = false;
                    set_h = false;
                    set_c = (value & 0x01) != 0;
                    break;
            }
            break;
        case 1: // BIT b, r
            set_z = ((value & (1 << bit)) == 0);
            set_n = false;
            set_h = true;
            set_c = cpu->get_flag(FLAG_C); // Preserve the carry flag
            result = value; // Don't modify the value for BIT
            break;
        case 2: // RES b, r
            result = value & ~(1 << bit);
            // No flags affected for RES
            break;
        case 3: // SET b, r
            result = value | (1 << bit);
            // No flags affected for SET
            break;
    }
    
    // Write the result back
    if (is_memory) {
        u16 addr = cpu->cpu_read_reg(RegType::HL);
        cpu->bus->write(addr, result);
        cpu->emu_cycles(1); // Memory write
    } else {
        cpu->cpu_set_reg(reg_type, result);
    }
    
    // Set flags if needed
    if (op == 0 || op == 1) { // Only set flags for rotation/shift and BIT operations
        cpu->set_flags(set_z, set_n, set_h, set_c);
    }
    
    cpu->emu_cycles(1); // Base cycle for CB instruction
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
        c = (int)(cpu->cpu_read_reg(inst->reg_1) & 0xFF) + (int)(cpu->fetched_data & 0xFF) >= 0x100;
    }

    cpu->cpu_set_reg(inst->reg_1, val & 0xFFFF);
    cpu->set_flags(z, 0, h, c);
}



static void proc_jr(CPU* cpu, const Instruction* inst) {
    char rel = (char)(cpu->fetched_data & 0xFF);
    u16 addr = cpu->regs.pc + rel;
    goto_addr(cpu, addr, false, inst->cond);
}

static void proc_di(CPU* cpu, const Instruction* inst) {
    cpu->ime = false;
}

static void proc_and(CPU* cpu, const Instruction* inst) {
    cpu->regs.a &= cpu->fetched_data & 0xFF;
    cpu->set_flags(cpu->regs.a == 0, 0, 1, 0);
}

static void proc_or(CPU* cpu, const Instruction* inst) {
    cpu->regs.a |= cpu->fetched_data & 0xFF;
    cpu->set_flags(cpu->regs.a == 0, 0, 0, 0);
}

static void proc_xor(CPU* cpu, const Instruction* inst) {
    // & 0xFF to ensure only lower 8 bits are used
    cpu->regs.a ^= cpu->fetched_data & 0xFF;
    cpu->set_flags(cpu->regs.a == 0, 0, 0, 0);
}

static void proc_cp(CPU* cpu, const Instruction* inst) {
    u8 operand1 = cpu->cpu_read_reg(inst->reg_1);
    u8 operand2 = cpu->fetched_data;
    u16 val = operand1 - operand2;
    cpu->set_flags(val == 0, 1, 
        (operand1 & 0x0F) < (operand2 & 0x0F),
        operand1 < operand2);
}

static void proc_rlca(CPU* cpu, const Instruction* inst) {
    // RLCA: Rotate A left through carry
    u8 a = cpu->regs.a;
    u8 bit7 = (a >> 7) & 1;  // Get the leftmost bit
    
    // Rotate left: shift left and put bit7 in bit0
    cpu->regs.a = ((a << 1) | bit7) & 0xFF;
    
    // Set flags: Z=0, N=0, H=0, C=bit7
    cpu->set_flags(0, 0, 0, bit7);
    cpu->emu_cycles(1);
}

static void proc_rrca(CPU* cpu, const Instruction* inst) {
    // RRCA: Rotate A right through carry
    u8 a = cpu->regs.a;
    u8 bit0 = a & 1;  // Get the rightmost bit
    
    // Rotate right: shift right and put bit0 in bit7
    cpu->regs.a = ((a >> 1) | (bit0 << 7)) & 0xFF;
    
    // Set flags: Z=0, N=0, H=0, C=bit0
    cpu->set_flags(0, 0, 0, bit0);
    cpu->emu_cycles(1);
}

static void proc_rla(CPU* cpu, const Instruction* inst) {
    // RLA: Rotate A left through carry
    u8 a = cpu->regs.a;
    u8 bit7 = (a >> 7) & 1;  // Get the leftmost bit
    u8 old_carry = cpu->get_flag(FLAG_C);
    
    // Rotate left: shift left and put old carry in bit0
    cpu->regs.a = ((a << 1) | old_carry) & 0xFF;
    
    // Set flags: Z=0, N=0, H=0, C=bit7
    cpu->set_flags(0, 0, 0, bit7);
    cpu->emu_cycles(1);
}

static void proc_rra(CPU* cpu, const Instruction* inst) {
    // RRA: Rotate A right through carry
    u8 a = cpu->regs.a;
    u8 bit0 = a & 1;  // Get the rightmost bit
    u8 old_carry = cpu->get_flag(FLAG_C);
    
    // Rotate right: shift right and put old carry in bit7
    cpu->regs.a = ((a >> 1) | (old_carry << 7)) & 0xFF;
    
    // Set flags: Z=0, N=0, H=0, C=bit0
    cpu->set_flags(0, 0, 0, bit0);
    cpu->emu_cycles(1);
}

static void proc_daa(CPU* cpu, const Instruction* inst) {
    if (!cpu->get_flag(FLAG_N)) {
        // After an addition, adjust if (half-)carry occurred or if result is out of bounds
        if (cpu->get_flag(FLAG_C) || cpu->regs.a > 0x99) {
            cpu->regs.a += 0x60;
            cpu->set_flag(FLAG_C, true);
        }
        if (cpu->get_flag(FLAG_H) || (cpu->regs.a & 0x0F) > 0x09) {
            cpu->regs.a += 0x06;
        }
    } else {
        // After a subtraction, only adjust if (half-)carry occurred
        if (cpu->get_flag(FLAG_C)) {
            cpu->regs.a -= 0x60;
            cpu->set_flag(FLAG_C, true);
        }
        if (cpu->get_flag(FLAG_H)) {
            cpu->regs.a -= 0x06;
        }
    }
    
    // These flags are always updated
    cpu->set_flags(cpu->regs.a == 0, cpu->get_flag(FLAG_N), 0, cpu->get_flag(FLAG_C));
}

static void proc_cpl(CPU* cpu, const Instruction* inst) {
    // CPL: Complement A (bitwise NOT)
    cpu->regs.a = ~cpu->regs.a;
    
    // Set flags: Z=unchanged, N=1, H=1, C=unchanged
    cpu->set_flags(cpu->get_flag(FLAG_Z), 1, 1, cpu->get_flag(FLAG_C));
}

static void proc_scf(CPU* cpu, const Instruction* inst) {
    // SCF: Set Carry Flag
    // Set flags: Z=unchanged, N=0, H=0, C=1
    cpu->set_flags(cpu->get_flag(FLAG_Z), 0, 0, 1);
}

static void proc_ccf(CPU* cpu, const Instruction* inst) {
    // CCF: Complement Carry Flag
    // Set flags: Z=unchanged, N=0, H=0, C=!C
    cpu->set_flags(cpu->get_flag(FLAG_Z), 0, 0, !cpu->get_flag(FLAG_C));
}

static void proc_halt(CPU* cpu, const Instruction* inst) {
    // HALT: Halt the CPU until an interrupt occurs
    cpu->halted = true;
    // No flags are affected by HALT
    // The CPU will remain halted until an interrupt occurs
}

static void proc_ei(CPU* cpu, const Instruction* inst) {
    // EI: Enable Interrupts
    // Enables interrupts after the next instruction
    cpu->enabling_ime = true;
    // No flags are affected by EI
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
}

static void proc_stop(CPU* cpu, const Instruction* inst) {
    cpu->emu_cycles(1);
}



static std::unordered_map<InType, InstrFunc> processors = {
    {InType::NONE, proc_none},
    {InType::NOP,  proc_nop},
    {InType::LD,   proc_ld},
    {InType::LDH,  proc_ldh},
    {InType::JP,   proc_jp},
    {InType::DI,   proc_di},
    {InType::AND,  proc_and},
    {InType::OR,   proc_or},
    {InType::XOR,  proc_xor},
    {InType::CP,   proc_cp},
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
    {InType::SBC,  proc_sbc},
    {InType::CB,   proc_cb},
    {InType::RLCA, proc_rlca},
    {InType::RRCA, proc_rrca},
    {InType::RLA,  proc_rla},
    {InType::RRA,  proc_rra},
    {InType::DAA,  proc_daa},
    {InType::CPL,  proc_cpl},
    {InType::SCF,  proc_scf},
    {InType::CCF,  proc_ccf},
    {InType::HALT, proc_halt},
    {InType::EI,   proc_ei}
};

InstrFunc inst_get_processor(InType type) {
    auto it = processors.find(type);
    if (it != processors.end()) {
        return it->second;
    }
    return proc_none; // Default to error handler
}




