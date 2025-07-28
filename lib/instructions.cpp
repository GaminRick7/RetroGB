#include "instructions.hpp"
#include <array>

// Static array to store all instructions indexed by opcode
static std::array<Instruction, 256> instruction_table = {};

// Initialize the instruction table
static void init_instruction_table() {
    // Initialize all to error instruction
    for (int i = 0; i < 256; i++) {
        instruction_table[i] = {
            InType::ERR,
            AddrMode::IMP,
            RegType::NONE,
            RegType::NONE,
            CondType::NONE,
            0
        };
    }

    // 0x00 - NOP
    instruction_table[0x00] = {InType::NOP, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // --- LD instructions ---
    // 8-bit LD r, r' and LD r, (HL) / LD (HL), r
    // r, r' in {B, C, D, E, H, L, (HL), A}
    RegType regs[8] = {RegType::B, RegType::C, RegType::D, RegType::E, RegType::H, RegType::L, RegType::NONE, RegType::A};
    for (int dst = 0; dst < 8; ++dst) {
        for (int src = 0; src < 8; ++src) {
            u8 opcode = 0x40 + dst * 8 + src;
            if (opcode == 0x76) continue; // HALT
            if (dst == 6 && src == 6) continue; // LD (HL), (HL) is invalid
            if (dst == 6) { // LD (HL), r
                instruction_table[opcode] = {InType::LD, AddrMode::MR_R, RegType::HL, regs[src], CondType::NONE, 0};
            } else if (src == 6) { // LD r, (HL)
                instruction_table[opcode] = {InType::LD, AddrMode::R_MR, regs[dst], RegType::HL, CondType::NONE, 0};
            } else { // LD r, r'
                instruction_table[opcode] = {InType::LD, AddrMode::R_R, regs[dst], regs[src], CondType::NONE, 0};
            }
        }
    }
    // 8-bit LD r, d8
    instruction_table[0x06] = {InType::LD, AddrMode::R_D8, RegType::B, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x0E] = {InType::LD, AddrMode::R_D8, RegType::C, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x16] = {InType::LD, AddrMode::R_D8, RegType::D, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x1E] = {InType::LD, AddrMode::R_D8, RegType::E, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x26] = {InType::LD, AddrMode::R_D8, RegType::H, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x2E] = {InType::LD, AddrMode::R_D8, RegType::L, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x3E] = {InType::LD, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0};
    // 8-bit LD (HL), d8
    instruction_table[0x36] = {InType::LD, AddrMode::MR_D8, RegType::HL, RegType::NONE, CondType::NONE, 0};
    // 16-bit LD rr, d16
    instruction_table[0x01] = {InType::LD, AddrMode::R_D16, RegType::BC, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x11] = {InType::LD, AddrMode::R_D16, RegType::DE, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x21] = {InType::LD, AddrMode::R_D16, RegType::HL, RegType::NONE, CondType::NONE, 0};
    instruction_table[0x31] = {InType::LD, AddrMode::R_D16, RegType::SP, RegType::NONE, CondType::NONE, 0};
    // LD SP, HL
    instruction_table[0xF9] = {InType::LD, AddrMode::R_R, RegType::SP, RegType::HL, CondType::NONE, 0};
    // LD HL, SP+e8
    instruction_table[0xF8] = {InType::LD, AddrMode::HL_SPR, RegType::HL, RegType::SP, CondType::NONE, 0};
    // LD (BC), A
    instruction_table[0x02] = {InType::LD, AddrMode::MR_R, RegType::BC, RegType::A, CondType::NONE, 0};
    // LD (DE), A
    instruction_table[0x12] = {InType::LD, AddrMode::MR_R, RegType::DE, RegType::A, CondType::NONE, 0};
    // LD A, (BC)
    instruction_table[0x0A] = {InType::LD, AddrMode::R_MR, RegType::A, RegType::BC, CondType::NONE, 0};
    // LD A, (DE)
    instruction_table[0x1A] = {InType::LD, AddrMode::R_MR, RegType::A, RegType::DE, CondType::NONE, 0};
    // LD (HL+), A
    instruction_table[0x22] = {InType::LD, AddrMode::HLI_R, RegType::HL, RegType::A, CondType::NONE, 0};
    // LD A, (HL+)
    instruction_table[0x2A] = {InType::LD, AddrMode::R_HLI, RegType::A, RegType::HL, CondType::NONE, 0};
    // LD (HL-), A
    instruction_table[0x32] = {InType::LD, AddrMode::HLD_R, RegType::HL, RegType::A, CondType::NONE, 0};
    // LD A, (HL-)
    instruction_table[0x3A] = {InType::LD, AddrMode::R_HLD, RegType::A, RegType::HL, CondType::NONE, 0};
    // LD (C), A
    instruction_table[0xE2] = {InType::LD, AddrMode::MR_R, RegType::C, RegType::A, CondType::NONE, 0};
    // LD A, (C)
    instruction_table[0xF2] = {InType::LD, AddrMode::R_MR, RegType::A, RegType::C, CondType::NONE, 0};
    // LDH (a8), A
    instruction_table[0xE0] = {InType::LDH, AddrMode::A8_R, RegType::NONE, RegType::A, CondType::NONE, 0};
    // LDH A, (a8)
    instruction_table[0xF0] = {InType::LDH, AddrMode::R_A8, RegType::A, RegType::NONE, CondType::NONE, 0};
    // LD (a16), A
    instruction_table[0xEA] = {InType::LD, AddrMode::A16_R, RegType::NONE, RegType::A, CondType::NONE, 0};
    // LD A, (a16)
    instruction_table[0xFA] = {InType::LD, AddrMode::R_A16, RegType::A, RegType::NONE, CondType::NONE, 0};
    // LD (a16), SP
    instruction_table[0x08] = {InType::LD, AddrMode::A16_R, RegType::NONE, RegType::SP, CondType::NONE, 0};
    
    // INC instructions
    // 0x03 - INC BC
    instruction_table[0x03] = {InType::INC, AddrMode::R, RegType::BC, RegType::NONE, CondType::NONE, 0};
    // 0x04 - INC B
    instruction_table[0x04] = {InType::INC, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0};
    // 0x0C - INC C
    instruction_table[0x0C] = {InType::INC, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0};
    // 0x13 - INC DE
    instruction_table[0x13] = {InType::INC, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0};
    // 0x14 - INC D
    instruction_table[0x14] = {InType::INC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0};
    // 0x1C - INC E
    instruction_table[0x1C] = {InType::INC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0};
    // 0x23 - INC HL
    instruction_table[0x23] = {InType::INC, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0};
    // 0x24 - INC H
    instruction_table[0x24] = {InType::INC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0};
    // 0x2C - INC L
    instruction_table[0x2C] = {InType::INC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0};
    // 0x33 - INC SP
    instruction_table[0x33] = {InType::INC, AddrMode::R, RegType::SP, RegType::NONE, CondType::NONE, 0};
    // 0x34 - INC (HL)
    instruction_table[0x34] = {InType::INC, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0};
    // 0x3C - INC A
    instruction_table[0x3C] = {InType::INC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0};

    // DEC instructions
    // 0x05 - DEC B
    instruction_table[0x05] = {InType::DEC, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0};
    // 0x0B - DEC BC
    instruction_table[0x0B] = {InType::DEC, AddrMode::R, RegType::BC, RegType::NONE, CondType::NONE, 0};
    // 0x0D - DEC C
    instruction_table[0x0D] = {InType::DEC, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0};
    // 0x15 - DEC D
    instruction_table[0x15] = {InType::DEC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0};
    // 0x1B - DEC DE
    instruction_table[0x1B] = {InType::DEC, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0};
    // 0x1D - DEC E
    instruction_table[0x1D] = {InType::DEC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0};
    // 0x25 - DEC H
    instruction_table[0x25] = {InType::DEC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0};
    // 0x2B - DEC HL
    instruction_table[0x2B] = {InType::DEC, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0};
    // 0x2D - DEC L
    instruction_table[0x2D] = {InType::DEC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0};
    // 0x35 - DEC (HL)
    instruction_table[0x35] = {InType::DEC, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0};
    // 0x3B - DEC SP
    instruction_table[0x3B] = {InType::DEC, AddrMode::R, RegType::SP, RegType::NONE, CondType::NONE, 0};
    // 0x3D - DEC A
    instruction_table[0x3D] = {InType::DEC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0};

    // // 0x07 - RLCA
    // instruction_table[0x07] = {InType::RLCA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x08 - LD (a16), SP
    instruction_table[0x08] = {InType::LD, AddrMode::A16_R, RegType::NONE, RegType::SP, CondType::NONE, 0};
    
    // // 0x09 - ADD HL, BC
    // instruction_table[0x09] = {InType::ADD, AddrMode::R_R, RegType::HL, RegType::BC, CondType::NONE, 0};
    
    // // 0x0A - LD A, (BC)
    instruction_table[0x0A] = {InType::LD, AddrMode::R_MR, RegType::A, RegType::BC, CondType::NONE, 0};
    
    // // 0x0B - DEC BC
    // instruction_table[0x0B] = {InType::DEC, AddrMode::R, RegType::BC, RegType::NONE, CondType::NONE, 0};
    
    // // 0x0F - RRCA
    // instruction_table[0x0F] = {InType::RRCA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x10 - STOP
    // instruction_table[0x10] = {InType::STOP, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x11 - LD DE, d16
    instruction_table[0x11] = {InType::LD, AddrMode::R_D16, RegType::DE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x12 - LD (DE), A
    instruction_table[0x12] = {InType::LD, AddrMode::MR_R, RegType::DE, RegType::A, CondType::NONE, 0};
    
    // // 0x13 - INC DE
    // instruction_table[0x13] = {InType::INC, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x14 - INC D
    // instruction_table[0x14] = {InType::INC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0};
    
    // // 0x15 - DEC D
    // instruction_table[0x15] = {InType::DEC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0};
    
    // // 0x16 - LD D, d8
    instruction_table[0x16] = {InType::LD, AddrMode::R_D8, RegType::D, RegType::NONE, CondType::NONE, 0};
    
    // // 0x17 - RLA
    // instruction_table[0x17] = {InType::RLA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x18 - JR r8
    // instruction_table[0x18] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x19 - ADD HL, DE
    // instruction_table[0x19] = {InType::ADD, AddrMode::R_R, RegType::HL, RegType::DE, CondType::NONE, 0};
    
    // // 0x1A - LD A, (DE)
    instruction_table[0x1A] = {InType::LD, AddrMode::R_MR, RegType::A, RegType::DE, CondType::NONE, 0};
    
    // // 0x1B - DEC DE
    // instruction_table[0x1B] = {InType::DEC, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x1C - INC E
    // instruction_table[0x1C] = {InType::INC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0};
    
    // // 0x1D - DEC E
    // instruction_table[0x1D] = {InType::DEC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0};
    
    // // 0x1E - LD E, d8
    instruction_table[0x1E] = {InType::LD, AddrMode::R_D8, RegType::E, RegType::NONE, CondType::NONE, 0};
    
    // // 0x1F - RRA
    // instruction_table[0x1F] = {InType::RRA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x20 - JR NZ, r8
    // instruction_table[0x20] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NZ, 0};
    
    // // 0x21 - LD HL, d16
    instruction_table[0x21] = {InType::LD, AddrMode::R_D16, RegType::HL, RegType::NONE, CondType::NONE, 0};
    
    // // 0x22 - LD (HL+), A
    instruction_table[0x22] = {InType::LD, AddrMode::HLI_R, RegType::HL, RegType::A, CondType::NONE, 0};
    
    // // 0x23 - INC HL
    // instruction_table[0x23] = {InType::INC, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0};
    
    // // 0x24 - INC H
    // instruction_table[0x24] = {InType::INC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0};
    
    // // 0x25 - DEC H
    instruction_table[0x25] = {InType::DEC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0};
    
    // // 0x26 - LD H, d8
    instruction_table[0x26] = {InType::LD, AddrMode::R_D8, RegType::H, RegType::NONE, CondType::NONE, 0};
    
    // // 0x27 - DAA
    // instruction_table[0x27] = {InType::DAA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x28 - JR Z, r8
    // instruction_table[0x28] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::Z, 0};
    
    // // 0x29 - ADD HL, HL
    // instruction_table[0x29] = {InType::ADD, AddrMode::R_R, RegType::HL, RegType::HL, CondType::NONE, 0};
    
    // // 0x2A - LD A, (HL+)
    instruction_table[0x2A] = {InType::LD, AddrMode::R_HLI, RegType::A, RegType::HL, CondType::NONE, 0};
    
    // // 0x2B - DEC HL
    // instruction_table[0x2B] = {InType::DEC, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0};
    
    // // 0x2C - INC L
    // instruction_table[0x2C] = {InType::INC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0};
    
    // // 0x2D - DEC L
    // instruction_table[0x2D] = {InType::DEC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0};
    
    // // 0x2E - LD L, d8
    instruction_table[0x2E] = {InType::LD, AddrMode::R_D8, RegType::L, RegType::NONE, CondType::NONE, 0};
    
    // // 0x2F - CPL
    // instruction_table[0x2F] = {InType::CPL, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x30 - JR NC, r8
    // instruction_table[0x30] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NC, 0};
    
    // // 0x31 - LD SP, d16
    instruction_table[0x31] = {InType::LD, AddrMode::R_D16, RegType::SP, RegType::NONE, CondType::NONE, 0};
    
    // // 0x32 - LD (HL-), A
    instruction_table[0x32] = {InType::LD, AddrMode::HLD_R, RegType::HL, RegType::A, CondType::NONE, 0};
    
    // // 0x33 - INC SP
    // instruction_table[0x33] = {InType::INC, AddrMode::R, RegType::SP, RegType::NONE, CondType::NONE, 0};
    
    // // 0x34 - INC (HL)
    // instruction_table[0x34] = {InType::INC, AddrMode::MR, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x35 - DEC (HL)
    instruction_table[0x35] = {InType::DEC, AddrMode::MR, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x36 - LD (HL), d8
    instruction_table[0x36] = {InType::LD, AddrMode::MR_D8, RegType::HL, RegType::NONE, CondType::NONE, 0};
    
    // // 0x37 - SCF
    // instruction_table[0x37] = {InType::SCF, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x38 - JR C, r8
    // instruction_table[0x38] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::C, 0};
    
    // // 0x39 - ADD HL, SP
    // instruction_table[0x39] = {InType::ADD, AddrMode::R_R, RegType::HL, RegType::SP, CondType::NONE, 0};
    
    // // 0x3A - LD A, (HL-)
    instruction_table[0x3A] = {InType::LD, AddrMode::R_HLD, RegType::A, RegType::HL, CondType::NONE, 0};
    
    // // 0x3B - DEC SP
    // instruction_table[0x3B] = {InType::DEC, AddrMode::R, RegType::SP, RegType::NONE, CondType::NONE, 0};
    
    // // 0x3C - INC A
    // instruction_table[0x3C] = {InType::INC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0};
    
    // // 0x3D - DEC A
    // instruction_table[0x3D] = {InType::DEC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0};
    
    // // 0x3E - LD A, d8
    instruction_table[0x3E] = {InType::LD, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0};
    
    // // 0x3F - CCF
    // instruction_table[0x3F] = {InType::CCF, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0x76 - HALT
    // instruction_table[0x76] = {InType::HALT, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0xCB - CB prefix (for extended instructions)
    // instruction_table[0xCB] = {InType::CB, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0xF3 - DI
    instruction_table[0xF3] = {InType::DI, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    
    // // 0xFB - EI
    // instruction_table[0xFB] = {InType::EI, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};

    // 0xC3 - JP a16
    instruction_table[0xC3] = {InType::JP, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NONE, 0};

    //0xEX
    instruction_table[0xE2] = {InType::LD, AddrMode::MR_R, RegType::C, RegType::A, CondType::NONE, 0},
    instruction_table[0xEA] = {InType::LD, AddrMode::A16_R, RegType::NONE, RegType::A, CondType::NONE, 0},


    //0xFX
    instruction_table[0xF2] = {InType::LD, AddrMode::R_MR, RegType::A, RegType::C, CondType::NONE, 0},
    instruction_table[0xF3] = {InType::DI, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0},
    instruction_table[0xFA] = {InType::LD, AddrMode::R_A16, RegType::A, RegType::NONE, CondType::NONE, 0},

    // 0xA8 - XOR B
    instruction_table[0xA8] = {InType::XOR, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0};
    // 0xA9 - XOR C
    instruction_table[0xA9] = {InType::XOR, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0};
    // 0xAA - XOR D
    instruction_table[0xAA] = {InType::XOR, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0};
    // 0xAB - XOR E
    instruction_table[0xAB] = {InType::XOR, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0};
    // 0xAC - XOR H
    instruction_table[0xAC] = {InType::XOR, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0};
    // 0xAD - XOR L
    instruction_table[0xAD] = {InType::XOR, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0};
    // 0xAE - XOR (HL)
    instruction_table[0xAE] = {InType::XOR, AddrMode::MR, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xAF - XOR A
    instruction_table[0xAF] = {InType::XOR, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0};
    // 0xEE - XOR d8
    instruction_table[0xEE] = {InType::XOR, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0};

    // POP instructions
    // 0xC1 - POP BC
    instruction_table[0xC1] = {InType::POP, AddrMode::IMP, RegType::BC, RegType::NONE, CondType::NONE, 0};
    // 0xD1 - POP DE
    instruction_table[0xD1] = {InType::POP, AddrMode::IMP, RegType::DE, RegType::NONE, CondType::NONE, 0};
    // 0xE1 - POP HL
    instruction_table[0xE1] = {InType::POP, AddrMode::IMP, RegType::HL, RegType::NONE, CondType::NONE, 0};
    // 0xF1 - POP AF
    instruction_table[0xF1] = {InType::POP, AddrMode::IMP, RegType::AF, RegType::NONE, CondType::NONE, 0};

    // PUSH instructions
    // 0xC5 - PUSH BC
    instruction_table[0xC5] = {InType::PUSH, AddrMode::IMP, RegType::BC, RegType::NONE, CondType::NONE, 0};
    // 0xD5 - PUSH DE
    instruction_table[0xD5] = {InType::PUSH, AddrMode::IMP, RegType::DE, RegType::NONE, CondType::NONE, 0};
    // 0xE5 - PUSH HL
    instruction_table[0xE5] = {InType::PUSH, AddrMode::IMP, RegType::HL, RegType::NONE, CondType::NONE, 0};
    // 0xF5 - PUSH AF
    instruction_table[0xF5] = {InType::PUSH, AddrMode::IMP, RegType::AF, RegType::NONE, CondType::NONE, 0};

    // CALL instructions
    // 0xC4 - CALL NZ,a16
    instruction_table[0xC4] = {InType::CALL, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NZ, 0};
    // 0xD4 - CALL NC,a16
    instruction_table[0xD4] = {InType::CALL, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NC, 0};
    // 0xCC - CALL Z,a16
    instruction_table[0xCC] = {InType::CALL, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::Z, 0};
    // 0xDC - CALL C,a16
    instruction_table[0xDC] = {InType::CALL, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::C, 0};
    // 0xCD - CALL a16
    instruction_table[0xCD] = {InType::CALL, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NONE, 0};

    // JR (Jump Relative) instructions
    // 0x20 - JR NZ,r8
    instruction_table[0x20] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NZ, 0};
    // 0x30 - JR NC,r8
    instruction_table[0x30] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NC, 0};
    // 0x18 - JR r8
    instruction_table[0x18] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0x28 - JR Z,r8
    instruction_table[0x28] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::Z, 0};
    // 0x38 - JR C,r8
    instruction_table[0x38] = {InType::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::C, 0};

    // JP (Jump) instructions
    // 0xC2 - JP NZ,a16
    instruction_table[0xC2] = {InType::JP, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NZ, 0};
    // 0xC3 - JP a16
    instruction_table[0xC3] = {InType::JP, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xD2 - JP NC,a16
    instruction_table[0xD2] = {InType::JP, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NC, 0};
    // 0xCA - JP Z,a16
    instruction_table[0xCA] = {InType::JP, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::Z, 0};
    // 0xDA - JP C,a16
    instruction_table[0xDA] = {InType::JP, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::C, 0};
    // 0xE9 - JP (HL)
    instruction_table[0xE9] = {InType::JP, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0};

    // RET (Return) instructions
    // 0xC0 - RET NZ
    instruction_table[0xC0] = {InType::RET, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NZ, 0};
    // 0xD0 - RET NC
    instruction_table[0xD0] = {InType::RET, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NC, 0};
    // 0xC8 - RET Z
    instruction_table[0xC8] = {InType::RET, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::Z, 0};
    // 0xD8 - RET C
    instruction_table[0xD8] = {InType::RET, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::C, 0};

    // Unconditional RET and RETI instructions
    // 0xC9 - RET
    instruction_table[0xC9] = {InType::RET, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xD9 - RETI
    instruction_table[0xD9] = {InType::RETI, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};

    // RST (Restart) instructions
    // 0xC7 - RST 00H
    instruction_table[0xC7] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xCF - RST 08H
    instruction_table[0xCF] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xD7 - RST 10H
    instruction_table[0xD7] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xDF - RST 18H
    instruction_table[0xDF] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xE7 - RST 20H
    instruction_table[0xE7] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xEF - RST 28H
    instruction_table[0xEF] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xF7 - RST 30H
    instruction_table[0xF7] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
    // 0xFF - RST 38H
    instruction_table[0xFF] = {InType::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0};
}

// Function to get instruction by opcode
Instruction* instruction_by_opcode(u8 opcode) {
    // Initialize table on first call
    static bool initialized = false;
    if (!initialized) {
        init_instruction_table();
        initialized = true;
    }
    
    return &instruction_table[opcode];
}

// Function to get instruction name
const char* inst_name(InType t) {
    switch (t) {
        case InType::NOP: return "NOP";
        case InType::LD: return "LD";
        case InType::INC: return "INC";
        case InType::DEC: return "DEC";
        case InType::RLCA: return "RLCA";
        case InType::ADD: return "ADD";
        case InType::RRCA: return "RRCA";
        case InType::STOP: return "STOP";
        case InType::RLA: return "RLA";
        case InType::JR: return "JR";
        case InType::RRA: return "RRA";
        case InType::DAA: return "DAA";
        case InType::CPL: return "CPL";
        case InType::SCF: return "SCF";
        case InType::CCF: return "CCF";
        case InType::HALT: return "HALT";
        case InType::ADC: return "ADC";
        case InType::SUB: return "SUB";
        case InType::SBC: return "SBC";
        case InType::AND: return "AND";
        case InType::XOR: return "XOR";
        case InType::OR: return "OR";
        case InType::CP: return "CP";
        case InType::POP: return "POP";
        case InType::JP: return "JP";
        case InType::PUSH: return "PUSH";
        case InType::RET: return "RET";
        case InType::CB: return "CB";
        case InType::CALL: return "CALL";
        case InType::RETI: return "RETI";
        case InType::LDH: return "LDH";
        case InType::JPHL: return "JPHL";
        case InType::DI: return "DI";
        case InType::EI: return "EI";
        case InType::RST: return "RST";
        case InType::RLC: return "RLC";
        case InType::RRC: return "RRC";
        case InType::RL: return "RL";
        case InType::RR: return "RR";
        case InType::SLA: return "SLA";
        case InType::SRA: return "SRA";
        case InType::SWAP: return "SWAP";
        case InType::SRL: return "SRL";
        case InType::BIT: return "BIT";
        case InType::RES: return "RES";
        case InType::SET: return "SET";
        case InType::ERR: return "ERR";
        case InType::NONE: return "NONE";
        default: return "UNKNOWN";
    }
} 