#pragma once

#include "common.hpp"

// ===== ADDRESSING MODES =====
enum class AddrMode {
    IMP,
    R_D16,
    R_R,
    MR_R,
    R,
    R_D8,
    R_MR,
    R_HLI,
    R_HLD,
    HLI_R,
    HLD_R,
    R_A8,
    A8_R,
    HL_SPR,
    D16,
    D8,
    D16_R,
    MR_D8,
    MR,
    A16_R,
    R_A16
};

// ===== REGISTER TYPES =====
enum class RegType {
    NONE,
    A,
    F,
    B,
    C,
    D,
    E,
    H,
    L,
    AF,
    BC,
    DE,
    HL,
    SP,
    PC
};

// ===== INSTRUCTION TYPES =====
enum class InType {
    NONE,
    NOP,
    LD,
    INC,
    DEC,
    RLCA,
    ADD,
    RRCA,
    STOP,
    RLA,
    JR,
    RRA,
    DAA,
    CPL,
    SCF,
    CCF,
    HALT,
    ADC,
    SUB,
    SBC,
    AND,
    XOR,
    OR,
    CP,
    POP,
    JP,
    PUSH,
    RET,
    CB,
    CALL,
    RETI,
    LDH,
    JPHL,
    DI,
    EI,
    RST,
    ERR,
    
    // CB instruction types
    RLC,
    RRC,
    RL,
    RR,
    SLA,
    SRA,
    SWAP,
    SRL,
    BIT,
    RES,
    SET
};

// ===== CONDITION TYPES =====
enum class CondType {
    NONE,
    NZ,
    Z,
    NC,
    C
};

// ===== INSTRUCTION STRUCTURE =====
struct Instruction {
    InType type;
    AddrMode mode;
    RegType reg_1;
    RegType reg_2;
    CondType cond;
    u8 param;
};

// ===== INSTRUCTION LOOKUP =====
/**
 * @brief Get instruction definition by opcode
 * @param opcode 8-bit instruction opcode
 * @return Pointer to instruction definition
 */
Instruction* instruction_by_opcode(u8 opcode);

/**
 * @brief Get instruction name as string
 * @param t Instruction type
 * @return String representation of instruction type
 */
const char* inst_name(InType t);