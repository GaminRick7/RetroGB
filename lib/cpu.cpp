#include "cpu.hpp"
#include "bus.hpp"
#include "cpu_exec.cpp"
#include <cstdio>

CPU::CPU() : bus(nullptr) {
    // Initialize CPU state
}

CPU::~CPU() {
    // Cleanup if needed
}

void CPU::init() {
    // Initialize CPU registers and state
    regs.pc = 0x0100;  // Start at the beginning of game code (after header)
    regs.sp = 0xFFFE;  // Stack pointer at top of high RAM
    regs.a = 0x01;
    regs.f = 0x00;
    regs.b = 0x00;
    regs.c = 0x00;
    regs.d = 0x00;
    regs.e = 0x00;
    regs.h = 0x00;
    regs.l = 0x00;
    
    // Initialize CPU state
    halted = false;
    stopped = false;
    cur_opcode = 0x00;
    curr_inst = nullptr;
    fetched_data = 0;
    mem_dest = 0;
    dest_is_mem = false;
    
    printf("CPU: Initialized - PC=0x%04X, SP=0x%04X\n", regs.pc, regs.sp);
}

void CPU::fetch_instruction() {
    if (!bus) {
        printf("CPU: No bus connected for instruction fetch!\n");
        return;
    }
    
    // Read the opcode from the current program counter
    cur_opcode = bus->read(regs.pc++);
    
    // Decode the opcode to get the instruction details
    curr_inst = instruction_by_opcode(cur_opcode);
    
    // Check if this is an error instruction
    if (curr_inst && curr_inst->type == InType::ERR) {
        printf("CPU: ERROR - Unknown opcode 0x%02X at PC=0x%04X\n", cur_opcode, regs.pc - 1);
        printf("CPU: Exiting due to unimplemented instruction\n");
        exit(1);  // Exit the program
    }
}

void CPU::fetch_data() {
    mem_dest = 0;
    dest_is_mem = false;
    switch (curr_inst->mode) {
        case AddrMode::IMP:{
            // No data to fetch for implied addressing
            fetched_data = 0;
            return;}
        case AddrMode::R:{
            fetched_data = cpu_read_reg(curr_inst->reg_1);
            return;}
        case AddrMode::D8:
        case AddrMode::R_D8:{
            fetched_data = bus->read(regs.pc);
            emu_cycles(1);
            regs.pc++;
            return;}
        case AddrMode::D16:
        case AddrMode::R_D16:{
            fetched_data = bus->read(regs.pc);
            fetched_data |= bus->read(regs.pc+1) << 8;
            emu_cycles(2);
            regs.pc +=2;
            return;}
        case AddrMode::R_R:{
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            return;}

        
        case AddrMode::MR_R:{
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            mem_dest = cpu_read_reg(curr_inst->reg_1);
            dest_is_mem = true;

            if (curr_inst->reg_1 == RegType::C){
                mem_dest |= 0xFF00;
            }
            return;}
        case AddrMode::R_MR:{
            u16 addr = cpu_read_reg(RegType::HL);

            if (curr_inst->reg_1 == RegType::C){
                addr |= 0xFF00;
            } 
            fetched_data = bus->read(addr);
            emu_cycles(1);
            return;}
        case AddrMode::R_HLI: {
            u16 addr = cpu_read_reg(RegType::HL);
            fetched_data = bus->read(addr);
            emu_cycles(1);
            // Increment HL after fetching
            u16 hl = cpu_read_reg(RegType::HL) + 1;
            // Set HL (split into H and L)
            regs.h = (hl >> 8) & 0xFF;
            regs.l = hl & 0xFF;
            return;
        }
        case AddrMode::R_HLD: {
            u16 addr = cpu_read_reg(RegType::HL);
            fetched_data = bus->read(addr);
            emu_cycles(1);
            // Decrement HL after fetching
            u16 hl = cpu_read_reg(RegType::HL) - 1;
            regs.h = (hl >> 8) & 0xFF;
            regs.l = hl & 0xFF;
            return;
        }
        case AddrMode::HLI_R: {
            u16 addr = cpu_read_reg(RegType::HL);
            mem_dest = addr;
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            // Increment HL after
            u16 hl = addr + 1;
            regs.h = (hl >> 8) & 0xFF;
            regs.l = hl & 0xFF;
            dest_is_mem = true;
            return;
        }
        case AddrMode::HLD_R: {
            u16 addr = cpu_read_reg(RegType::HL);
            mem_dest = addr;
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            // Decrement HL after
            u16 hl = addr - 1;
            regs.h = (hl >> 8) & 0xFF;
            regs.l = hl & 0xFF;
            dest_is_mem = true;
            return;
        }
        case AddrMode::R_A8: {
            fetched_data = bus->read(regs.pc);
            regs.pc++;
            emu_cycles(1);
            return;
        }
        case AddrMode::A8_R: {
            mem_dest = 0xFF00 | bus->read(regs.pc);
            regs.pc++;
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            dest_is_mem = true;
            emu_cycles(1);
            return;
        }
        case AddrMode::HL_SPR: {
            fetched_data = bus->read(regs.pc);
            regs.pc++;
            emu_cycles(1);
            return;
        }
        case AddrMode::A16_R:
        case AddrMode::D16_R: {
            u16 addr = bus->read(regs.pc) | (bus->read(regs.pc + 1) << 8);
            regs.pc += 2;
            mem_dest = addr;
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            dest_is_mem = true;
            emu_cycles(2);
            return;
        }
        case AddrMode::MR_D8: {
            fetched_data = bus->read(regs.pc);
            regs.pc++;
            mem_dest = cpu_read_reg(curr_inst->reg_1);
            dest_is_mem = true;
            emu_cycles(1);
            return;
        }
        case AddrMode::MR: {
            mem_dest = cpu_read_reg(curr_inst->reg_1);
            dest_is_mem = true;
            fetched_data = bus->read(mem_dest);
            emu_cycles(1);
            return;
        }
        case AddrMode::R_A16: {
            u16 addr = bus->read(regs.pc) | (bus->read(regs.pc + 1) << 8);
            regs.pc += 2;
            fetched_data = bus->read(addr);
            emu_cycles(2);
            return;
        }
        default:
            // For unimplemented addressing modes, just set to 0
            fetched_data = 0;
            return;
    }
}

void CPU::emu_cycles(int cycles) {
    // Empty function for cycle counting
    // This will be implemented later to track CPU cycles
    // and synchronize with other components (PPU, timer, etc.)
}

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
            // Return full 16-bit AF value (F << 8 | A) - little-endian
            return (regs.f << 8) | regs.a;
        case RegType::BC:
            // Return full 16-bit BC value (C << 8 | B) - little-endian
            return (regs.c << 8) | regs.b;
        case RegType::DE:
            // Return full 16-bit DE value (E << 8 | D) - little-endian
            return (regs.e << 8) | regs.d;
        case RegType::HL:
            // Return full 16-bit HL value (L << 8 | H) - little-endian
            return (regs.l << 8) | regs.h;
        case RegType::SP:
            // Return full 16-bit SP value
            return regs.sp;
        case RegType::PC:
            // Return full 16-bit PC value
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
            regs.a = value & 0xFF;
            regs.f = (value >> 8) & 0xFF;
            break;
        case RegType::BC:
            regs.b = value & 0xFF;
            regs.c = (value >> 8) & 0xFF;
            break;
        case RegType::DE:
            regs.d = value & 0xFF;
            regs.e = (value >> 8) & 0xFF;
            break;
        case RegType::HL:
            regs.h = value & 0xFF;
            regs.l = (value >> 8) & 0xFF;
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

void CPU::execute() {
    if (!curr_inst) {
        printf("CPU: No instruction to execute!\n");
        return;
    }
    InstrFunc func = inst_get_processor(curr_inst->type);
    if (func) {
        func(this, curr_inst);
    } else {
        printf("CPU: No processor for instruction type %d!\n", static_cast<int>(curr_inst->type));
    }
}

bool CPU::step() {
    static int instruction_count = 0;
    if (!halted) {
        instruction_count++;
        printf("Step %d: PC=0x%04X ", instruction_count, regs.pc);
        fetch_instruction();
        fetch_data();
        printf("0x%02X(%s)", cur_opcode, curr_inst ? inst_name(curr_inst->type) : "UNK");
        execute();
        printf("%*sA:%02X BC:%02X%02X DE:%02X%02X HL:%02X%02X SP:%04X\n", 
               20, "", regs.a, regs.b, regs.c, regs.d, regs.e, regs.h, regs.l, regs.sp);
    }
    return true;  // Continue running instructions
} 

// Flag helper implementations

void CPU::set_flags(u8 z, u8 n, u8 h, u8 c) {
    set_flag(FLAG_Z, z);
    set_flag(FLAG_N, n);
    set_flag(FLAG_H, h);
    set_flag(FLAG_C, c);
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

u8 CPU::get_ie_register() const {
    return ie_register;
}

void CPU::set_ie_register(u8 value) {
    ie_register = value;
} 

void CPU::stack_push(u8 value) {
    regs.sp--;
    bus->write(regs.sp, value);
}

void CPU::stack_push16(u16 value) {
    regs.sp--;
    bus->write(regs.sp, value & 0xFF);
    bus->write(regs.sp + 1, (value >> 8) & 0xFF);
}

u16 CPU::stack_pop() {
    return bus->read(regs.sp++);
}

u8 CPU::stack_pop16() {
    u16 value = bus->read(regs.sp) | (bus->read(regs.sp + 1) << 8);
    regs.sp += 2;
    return value;
}
u16 CPU::stack_peek() {
    return bus->read(regs.sp) | (bus->read(regs.sp + 1) << 8);
}