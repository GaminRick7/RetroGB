#include "cpu.hpp"
#include "bus.hpp"
#include "ppu.hpp"
#include <cstdio>
#include <cstring>

#define CPU_DEBUG 0

// Helper function to get register name as string
static const char* reg_name(RegType reg) {
    switch (reg) {
        case RegType::A: return "A";
        case RegType::B: return "B";
        case RegType::C: return "C";
        case RegType::D: return "D";
        case RegType::E: return "E";
        case RegType::H: return "H";
        case RegType::L: return "L";
        case RegType::F: return "F";
        case RegType::AF: return "AF";
        case RegType::BC: return "BC";
        case RegType::DE: return "DE";
        case RegType::HL: return "HL";
        case RegType::SP: return "SP";
        case RegType::PC: return "PC";
        case RegType::NONE: return "";
        default: return "?";
    }
}

// Helper function to get condition name as string
static const char* cond_name(CondType cond) {
    switch (cond) {
        case CondType::NZ: return "NZ";
        case CondType::Z: return "Z";
        case CondType::NC: return "NC";
        case CondType::C: return "C";
        case CondType::NONE: return "";
        default: return "?";
    }
}

// Helper function to format operand based on addressing mode and instruction
static void format_operand(const Instruction* inst, u16 fetched_data, u16 mem_dest, char* buffer, size_t buffer_size) {
    switch (inst->mode) {
        case AddrMode::IMP:
            // Special handling for POP and PUSH instructions
            if (inst->type == InType::POP || inst->type == InType::PUSH) {
                snprintf(buffer, buffer_size, "%s", reg_name(inst->reg_1));
            } else {
                snprintf(buffer, buffer_size, "");
            }
            break;
        case AddrMode::R:
            snprintf(buffer, buffer_size, "%s", reg_name(inst->reg_1));
            break;
        case AddrMode::R_R:
            snprintf(buffer, buffer_size, "%s,%s", reg_name(inst->reg_1), reg_name(inst->reg_2));
            break;
        case AddrMode::R_D8:
            snprintf(buffer, buffer_size, "%s,$%02X", reg_name(inst->reg_1), fetched_data);
            break;
        case AddrMode::R_D16:
            snprintf(buffer, buffer_size, "%s,$%04X", reg_name(inst->reg_1), fetched_data);
            break;
        case AddrMode::MR_R:
            if (inst->reg_1 == RegType::C) {
                snprintf(buffer, buffer_size, "($FF00+C),%s", reg_name(inst->reg_2));
            } else {
                snprintf(buffer, buffer_size, "(%s),%s", reg_name(inst->reg_1), reg_name(inst->reg_2));
            }
            break;
        case AddrMode::R_MR:
            if (inst->reg_1 == RegType::C) {
                snprintf(buffer, buffer_size, "%s,($FF00+C)", reg_name(inst->reg_2));
            } else {
                snprintf(buffer, buffer_size, "%s,(%s)", reg_name(inst->reg_1), reg_name(inst->reg_2));
            }
            break;
        case AddrMode::R_HLI:
            snprintf(buffer, buffer_size, "%s,(HL+)", reg_name(inst->reg_1));
            break;
        case AddrMode::R_HLD:
            snprintf(buffer, buffer_size, "%s,(HL-)", reg_name(inst->reg_1));
            break;
        case AddrMode::HLI_R:
            snprintf(buffer, buffer_size, "(HL+),%s", reg_name(inst->reg_2));
            break;
        case AddrMode::HLD_R:
            snprintf(buffer, buffer_size, "(HL-),%s", reg_name(inst->reg_2));
            break;
        case AddrMode::R_A8:
            snprintf(buffer, buffer_size, "%s,($FF00+$%02X)", reg_name(inst->reg_1), fetched_data);
            break;
        case AddrMode::A8_R:
            snprintf(buffer, buffer_size, "($%04X),%s", mem_dest, reg_name(inst->reg_2));
            break;
        case AddrMode::HL_SPR:
            snprintf(buffer, buffer_size, "HL,SP+%d", (int8_t)fetched_data);
            break;
        case AddrMode::D8:
            snprintf(buffer, buffer_size, "$%02X", fetched_data);
            break;
        case AddrMode::D16:
            snprintf(buffer, buffer_size, "$%04X", fetched_data);
            break;
        case AddrMode::D16_R:
            snprintf(buffer, buffer_size, "($%04X),%s", fetched_data, reg_name(inst->reg_2));
            break;
        case AddrMode::MR_D8:
            snprintf(buffer, buffer_size, "(%s),$%02X", reg_name(inst->reg_1), fetched_data);
            break;
        case AddrMode::MR:
            snprintf(buffer, buffer_size, "(%s)", reg_name(inst->reg_1));
            break;
        case AddrMode::A16_R:
            snprintf(buffer, buffer_size, "($%04X),%s", fetched_data, reg_name(inst->reg_2));
            break;
        case AddrMode::R_A16:
            snprintf(buffer, buffer_size, "%s,($%04X)", reg_name(inst->reg_1), fetched_data);
            break;
        default:
            snprintf(buffer, buffer_size, "?");
            break;
    }
}

// ===== CONSTRUCTORS & DESTRUCTORS =====

CPU::CPU() : bus(nullptr) {
    // Initialize CPU state
}

CPU::~CPU() {
    // Cleanup if needed
}

// ===== INITIALIZATION =====

void CPU::init() {
    // Initialize CPU registers and state according to Gameboy Doctor requirements
    regs.pc = 0x0100;  // Start at the beginning of game code (after header)
    regs.sp = 0xFFFE;  // Stack pointer at top of high RAM
    regs.a  = 0x01;
    regs.f  = 0xB0;
    regs.b  = 0x00;
    regs.c  = 0x13;
    regs.d  = 0x00;
    regs.e  = 0xD8;
    regs.h  = 0x01;
    regs.l  = 0x4D;
    int_flags = 0;
    ie_register = 0;
    ime = false;
    enabling_ime = false;
    ticks = 0;
    
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

// ===== INSTRUCTION FETCHING =====

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
            u16 lo = bus->read(regs.pc);
            emu_cycles(1);

            u16 hi = bus->read(regs.pc + 1);
            emu_cycles(1);

            fetched_data = lo | (hi << 8);

            regs.pc += 2;
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
            u16 addr = cpu_read_reg(curr_inst->reg_2);

            if (curr_inst->reg_2 == RegType::C){
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
            emu_cycles(1);
            regs.pc++;
            return;
        }
        case AddrMode::A8_R: {
            mem_dest = 0xFF00 | bus->read(regs.pc);
            emu_cycles(1);
            regs.pc++;
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            dest_is_mem = true;
            return;
        }
        case AddrMode::HL_SPR: {
            fetched_data = bus->read(regs.pc);
            emu_cycles(1);
            regs.pc++;
            return;
        }
        case AddrMode::A16_R:
        case AddrMode::D16_R: {
            u16 lo = bus->read(regs.pc);
            emu_cycles(1);

            u16 hi = bus->read(regs.pc + 1);
            emu_cycles(1);

            mem_dest = lo | (hi << 8);
            dest_is_mem = true;

            regs.pc += 2;
            fetched_data = cpu_read_reg(curr_inst->reg_2);
            return;
        }
        case AddrMode::MR_D8: {
            fetched_data = bus->read(regs.pc);
            emu_cycles(1);
            regs.pc++;
            mem_dest = cpu_read_reg(curr_inst->reg_1);
            dest_is_mem = true;
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
            u16 lo = bus->read(regs.pc);
            emu_cycles(1);

            u16 hi = bus->read(regs.pc + 1);
            emu_cycles(1);

            u16 addr = lo | (hi << 8);

            regs.pc += 2;
            fetched_data = bus->read(addr);
            emu_cycles(1);
            return;
        }
        default:
            // For unimplemented addressing modes, just set to 0
            fetched_data = 0;
            return;
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

// ===== CYCLE MANAGEMENT =====

void CPU::emu_cycles(int cycles) {
    // Empty function for cycle counting
    // This will be implemented later to track CPU cycles
    // and synchronize with other components (PPU, timer, etc.)

    for (int i=0; i<cycles; i++) {
        for (int n=0; n<4; n++) {
            ticks++;
            timer->tick();
            ppu->tick();
        }

        dma->tick();
    }
}

// ===== MAIN EXECUTION =====

bool CPU::step() {
    
    // Gameboy Doctor logging - log CPU state BEFORE instruction execution
        // Only log if CPU is not halted and executed an opcode
        if (!halted) {
            #if CPU_DEBUG
            // Read 4 bytes from PC for PCMEM
            u8 pcmem0 = bus->read(regs.pc);
            u8 pcmem1 = bus->read(regs.pc + 1);
            u8 pcmem2 = bus->read(regs.pc + 2);
            u8 pcmem3 = bus->read(regs.pc + 3);
            
            // Log in Gameboy Doctor format: A:00 F:11 B:22 C:33 D:44 E:55 H:66 L:77 SP:8888 PC:9999 PCMEM:AA,BB,CC,DD
            // F field should be hex value of flags register (not string)
            printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
                    regs.a, regs.f, regs.b, regs.c, regs.d, regs.e, regs.h, regs.l, regs.sp, regs.pc,
                    pcmem0, pcmem1, pcmem2, pcmem3);
            #endif
        }
        
        // Don't increment PC here - it should be incremented after instruction execution
        // The PC increment logic will be handled in the instruction processors
    
    if (!halted) {
        u16 pc = regs.pc;
        fetch_instruction();
        emu_cycles(1);
        fetch_data();
        #if CPU_DEBUG
        // Format the instruction with operands
        char operand_buffer[64];
        format_operand(curr_inst, fetched_data, mem_dest, operand_buffer, sizeof(operand_buffer));
        
        // Handle special cases for conditional instructions
        const char* condition = "";
        if (curr_inst->cond != CondType::NONE) {
            condition = cond_name(curr_inst->cond);
        }
        
        // Compose the instruction string into a buffer
        char instr_buffer[128];
        int instr_len = 0;
        if (curr_inst->type == InType::RST) {
            u8 rst_addr = (cur_opcode & 0x38) >> 3;
            instr_len = snprintf(instr_buffer, sizeof(instr_buffer),
                "Step %d: PC=0x%04X 0x%02X %s%s $%02X",
                ticks, pc, cur_opcode, inst_name(curr_inst->type), condition, rst_addr * 8);
        } else if (curr_inst->type == InType::JR) {
            int8_t rel_addr = (int8_t)fetched_data;
            u16 target_addr = regs.pc + rel_addr;
            instr_len = snprintf(instr_buffer, sizeof(instr_buffer),
                "Step %d: PC=0x%04X 0x%02X %s%s $%+d",
                ticks, pc, cur_opcode, inst_name(curr_inst->type), condition, rel_addr);
        } else if (curr_inst->type == InType::JP || curr_inst->type == InType::CALL) {
            instr_len = snprintf(instr_buffer, sizeof(instr_buffer),
                "Step %d: PC=0x%04X 0x%02X %s%s $%04X",
                ticks, pc, cur_opcode, inst_name(curr_inst->type), condition, fetched_data);
        } else if (curr_inst->type == InType::JPHL) {
            instr_len = snprintf(instr_buffer, sizeof(instr_buffer),
                "Step %d: PC=0x%04X 0x%02X %s (HL)",
                ticks, pc, cur_opcode, inst_name(curr_inst->type));
        } else {
            instr_len = snprintf(instr_buffer, sizeof(instr_buffer),
                "Step %d: PC=0x%04X 0x%02X %s%s %s",
                ticks, pc, cur_opcode, inst_name(curr_inst->type), condition, operand_buffer);
        }

        // Print the instruction part
        printf("%s", instr_buffer);

        // Calculate how many spaces to print to align registers at column 40
        int reg_column = 40;
        if (instr_len < reg_column) {
            for (int i = 0; i < reg_column - instr_len; ++i) putchar(' ');
        } else {
            putchar(' '); // Always at least one space
        }

        // Get flag states
        bool z_flag = get_flag(FLAG_Z);
        bool n_flag = get_flag(FLAG_N);
        bool h_flag = get_flag(FLAG_H);
        bool c_flag = get_flag(FLAG_C);

        // Print registers
        printf("A:%02X BC:%02X%02X DE:%02X%02X HL:%02X%02X SP:%04X F:%02X[%c%c%c%c]\n",
               regs.a, regs.b, regs.c, regs.d, regs.e, regs.h, regs.l, regs.sp,
               regs.f, z_flag ? 'Z' : '-', n_flag ? 'N' : '-', h_flag ? 'H' : '-', c_flag ? 'C' : '-');
        
        
        dbg_update();
        dbg_print();

        #endif
        execute();

    }
    else {
        emu_cycles(1);
        
        if (int_flags) {
            halted = false;
        }
    }

    if (ime) {
        handle_interrupts();
        enabling_ime = false;
    }
    if (enabling_ime) {
        ime = true;
    }
    return true;  // Continue running instructions
} 

// ===== DEBUG FUNCTIONS =====

void CPU::dbg_update() {
    if (bus->read(0xFF02) == 0x81) {
        char c = bus->read(0xFF01);

        dbg_msg[msg_size++] = c;
        bus->write(0xFF02, 0);
    }
}

void CPU::dbg_print() {
    if (dbg_msg[0]) {
        printf("DBG: %s\n", dbg_msg);
        
        // Check if the debug message contains "failed"
        if (strstr(dbg_msg, "Passed") != nullptr) {
            printf("CPU: Debug message contains 'failed' - stopping program\n");
            exit(1);
        }
    }
}
