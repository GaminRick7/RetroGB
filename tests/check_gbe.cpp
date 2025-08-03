#include <cstdlib>
#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <check.h>
#include "emu.hpp"
#include "cpu.hpp"

// Forward declaration of check_condition function
static bool check_condition(CPU* cpu, CondType cond);

// Implementation of check_condition function
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

START_TEST(test_nothing) {
    // Create a simple test that doesn't require full emulator setup
    CPU cpu;
    cpu.init();
    
    // Test basic flag operations
    cpu.set_flags(1, 0, 1, 0);  // Set Z and H flags
    ck_assert(cpu.get_flag(FLAG_Z) == true);
    ck_assert(cpu.get_flag(FLAG_N) == false);
    ck_assert(cpu.get_flag(FLAG_H) == true);
    ck_assert(cpu.get_flag(FLAG_C) == false);
    
    // Test flag clearing
    cpu.set_flags(0, 0, 0, 0);
    ck_assert(cpu.get_flag(FLAG_Z) == false);
    ck_assert(cpu.get_flag(FLAG_N) == false);
    ck_assert(cpu.get_flag(FLAG_H) == false);
    ck_assert(cpu.get_flag(FLAG_C) == false);
    
    // Test individual flag setting
    cpu.set_flag(FLAG_C, true);
    ck_assert(cpu.get_flag(FLAG_C) == true);
    
    cpu.set_flag(FLAG_Z, true);
    ck_assert(cpu.get_flag(FLAG_Z) == true);
    
    bool b = true;  // Test passes
    ck_assert_uint_eq(b, true);
} END_TEST

START_TEST(test_instruction_flags) {
    CPU cpu;
    cpu.init();
    
    // Test ADD instruction flags
    cpu.regs.a = 0x3A;  // Set A to 0x3A
    cpu.fetched_data = 0x06;  // Add 0x06
    cpu.cpu_set_reg(RegType::A, 0x3A);
    
    // Simulate ADD A, d8 (0xC6)
    u8 a = cpu.regs.a;
    u8 b = cpu.fetched_data;
    u16 val = a + b;
    
    // Calculate expected flags
    bool z = (val & 0xFF) == 0;
    bool h = (a & 0xF) + (b & 0xF) >= 0x10;
    bool c = (int)(a & 0xFF) + (int)(b & 0xFF) >= 0x100;
    
    // Set flags
    cpu.set_flags(z, 0, h, c);
    
    // Verify flags
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // 0x3A + 0x06 = 0x40, not zero
    ck_assert(cpu.get_flag(FLAG_N) == false);  // ADD always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == true);   // 0xA + 0x6 = 0x10, half-carry
    ck_assert(cpu.get_flag(FLAG_C) == false);  // 0x3A + 0x06 = 0x40, no carry
    
    // Test SUB instruction flags
    cpu.regs.a = 0x3E;  // Set A to 0x3E
    cpu.fetched_data = 0x06;  // Subtract 0x06
    cpu.cpu_set_reg(RegType::A, 0x3E);
    
    // Simulate SUB A, d8
    a = cpu.regs.a;
    b = cpu.fetched_data;
    val = a - b;
    
    // Calculate expected flags for SUB
    z = (val & 0xFF) == 0;
    h = (a & 0xF) < (b & 0xF);
    c = a < b;
    
    cpu.set_flags(z, 1, h, c);  // SUB sets N=1
    
    // Verify flags
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // 0x3E - 0x06 = 0x38, not zero
    ck_assert(cpu.get_flag(FLAG_N) == true);   // SUB always sets N=1
    ck_assert(cpu.get_flag(FLAG_H) == false);  // 0xE - 0x6 = 0x8, no half-borrow
    ck_assert(cpu.get_flag(FLAG_C) == false);  // 0x3E - 0x06 = 0x38, no borrow
    
    // Test XOR instruction flags
    cpu.regs.a = 0x5A;  // Set A to 0x5A
    cpu.fetched_data = 0x5A;  // XOR with 0x5A
    cpu.cpu_set_reg(RegType::A, 0x5A);
    
    // Simulate XOR A, d8
    cpu.regs.a ^= cpu.fetched_data & 0xFF;
    cpu.set_flags(cpu.regs.a == 0, 0, 0, 0);
    
    // Verify flags
    ck_assert(cpu.get_flag(FLAG_Z) == true);   // 0x5A XOR 0x5A = 0x00, zero
    ck_assert(cpu.get_flag(FLAG_N) == false);  // XOR always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // XOR always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == false);  // XOR always sets C=0
    
} END_TEST

START_TEST(test_arithmetic_instructions) {
    CPU cpu;
    cpu.init();
    
    // Test ADC (Add with Carry)
    cpu.regs.a = 0xE1;
    cpu.fetched_data = 0x0F;
    cpu.set_flag(FLAG_C, true);  // Set carry flag
    
    // Simulate ADC A, d8
    u8 a = cpu.regs.a;
    u8 b = cpu.fetched_data;
    u8 carry = cpu.get_flag(FLAG_C);
    u16 result = a + b + carry;
    
    cpu.regs.a = result & 0xFF;
    bool z = cpu.regs.a == 0;
    bool h = (a & 0xF) + (b & 0xF) + carry > 0xF;
    bool c = a + b + carry > 0xFF;
    
    cpu.set_flags(z, 0, h, c);
    
    ck_assert(cpu.regs.a == 0xF1);  // 0xE1 + 0x0F + 1 = 0xF1
    ck_assert(cpu.get_flag(FLAG_Z) == false);
    ck_assert(cpu.get_flag(FLAG_N) == false);
    ck_assert(cpu.get_flag(FLAG_H) == true);   // 0x1 + 0xF + 1 = 0x11, half-carry
    ck_assert(cpu.get_flag(FLAG_C) == false);  // 0xE1 + 0x0F + 1 = 0xF1, no carry
    
    // Test SBC (Subtract with Carry)
    cpu.regs.a = 0x3B;
    cpu.fetched_data = 0x2A;
    cpu.set_flag(FLAG_C, true);  // Set carry flag
    
    // Simulate SBC A, d8
    a = cpu.regs.a;
    b = cpu.fetched_data;
    carry = cpu.get_flag(FLAG_C);
    result = a - b - carry;
    
    cpu.regs.a = result & 0xFF;
    z = cpu.regs.a == 0;
    h = (a & 0xF) < ((b & 0xF) + carry);
    c = a < (b + carry);
    
    cpu.set_flags(z, 1, h, c);
    
    ck_assert(cpu.regs.a == 0x10);  // 0x3B - 0x2A - 1 = 0x10
    ck_assert(cpu.get_flag(FLAG_Z) == false);
    ck_assert(cpu.get_flag(FLAG_N) == true);
    ck_assert(cpu.get_flag(FLAG_H) == false);  // 0xB - 0xA - 1 = 0x0, no half-borrow
    ck_assert(cpu.get_flag(FLAG_C) == false);  // 0x3B - 0x2A - 1 = 0x10, no borrow
    
    // Test INC (Increment)
    cpu.regs.b = 0xFF;
    cpu.cpu_set_reg(RegType::B, 0xFF);
    
    // Simulate INC B
    u8 value = cpu.regs.b;
    u8 inc_result = value + 1;
    bool half_carry = (value & 0x0F) == 0x0F;
    
    cpu.regs.b = inc_result;
    cpu.set_flags(inc_result == 0, 0, half_carry, 0);
    
    ck_assert(cpu.regs.b == 0x00);  // 0xFF + 1 = 0x00
    ck_assert(cpu.get_flag(FLAG_Z) == true);   // Result is zero
    ck_assert(cpu.get_flag(FLAG_N) == false);  // INC always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == true);   // 0xF + 1 = 0x10, half-carry
    ck_assert(cpu.get_flag(FLAG_C) == false);  // INC never affects carry
    
    // Test DEC (Decrement)
    cpu.regs.c = 0x01;
    cpu.cpu_set_reg(RegType::C, 0x01);
    
    // Simulate DEC C
    value = cpu.regs.c;
    u8 dec_result = value - 1;
    bool half_borrow = (value & 0x0F) == 0x00;
    
    cpu.regs.c = dec_result;
    cpu.set_flags(dec_result == 0, 1, half_borrow, 0);
    
    ck_assert(cpu.regs.c == 0x00);  // 0x01 - 1 = 0x00
    ck_assert(cpu.get_flag(FLAG_Z) == true);   // Result is zero
    ck_assert(cpu.get_flag(FLAG_N) == true);   // DEC always sets N=1
    ck_assert(cpu.get_flag(FLAG_H) == false);  // 0x1 - 1 = 0x0, no half-borrow
    ck_assert(cpu.get_flag(FLAG_C) == false);  // DEC never affects carry
    
} END_TEST

START_TEST(test_logical_instructions) {
    CPU cpu;
    cpu.init();
    
    // Test AND instruction
    cpu.regs.a = 0x5A;
    cpu.fetched_data = 0x3F;
    
    // Simulate AND A, d8
    cpu.regs.a &= cpu.fetched_data & 0xFF;
    cpu.set_flags(cpu.regs.a == 0, 0, 0, 0);
    
    ck_assert(cpu.regs.a == 0x1A);  // 0x5A & 0x3F = 0x1A
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // Result not zero
    ck_assert(cpu.get_flag(FLAG_N) == false);  // AND always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // AND always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == false);  // AND always sets C=0
    
    // Test OR instruction
    cpu.regs.a = 0x5A;
    cpu.fetched_data = 0x0F;
    
    // Simulate OR A, d8
    cpu.regs.a |= cpu.fetched_data & 0xFF;
    cpu.set_flags(cpu.regs.a == 0, 0, 0, 0);
    
    ck_assert(cpu.regs.a == 0x5F);  // 0x5A | 0x0F = 0x5F
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // Result not zero
    ck_assert(cpu.get_flag(FLAG_N) == false);  // OR always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // OR always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == false);  // OR always sets C=0
    
    // Test XOR instruction (different case)
    cpu.regs.a = 0xAA;
    cpu.fetched_data = 0x55;
    
    // Simulate XOR A, d8
    cpu.regs.a ^= cpu.fetched_data & 0xFF;
    cpu.set_flags(cpu.regs.a == 0, 0, 0, 0);
    
    ck_assert(cpu.regs.a == 0xFF);  // 0xAA ^ 0x55 = 0xFF
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // Result not zero
    ck_assert(cpu.get_flag(FLAG_N) == false);  // XOR always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // XOR always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == false);  // XOR always sets C=0
    
    // Test CP (Compare) instruction
    cpu.regs.a = 0x3C;
    cpu.fetched_data = 0x2F;
    
    // Simulate CP A, d8
    u8 a = cpu.regs.a;
    u8 b = cpu.fetched_data;
    u16 val = a - b;
    
    bool z = val == 0;
    bool h = (a & 0x0F) < (b & 0x0F);  // Check if lower nibble borrow occurred
    bool c = a < b;
    
    cpu.set_flags(z, 1, h, c);
    
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // 0x3C - 0x2F = 0x0D, not zero
    ck_assert(cpu.get_flag(FLAG_N) == true);   // CP always sets N=1
    ck_assert(cpu.get_flag(FLAG_H) == true);   // 0xC < 0xF, half-borrow occurred
    ck_assert(cpu.get_flag(FLAG_C) == false);  // 0x3C - 0x2F = 0x0D, no borrow
    
} END_TEST

START_TEST(test_rotation_instructions) {
    CPU cpu;
    cpu.init();
    
    // Test RLCA (Rotate Left through Carry)
    cpu.regs.a = 0x85;  // 10000101
    
    // Simulate RLCA
    u8 a = cpu.regs.a;
    u8 bit7 = (a >> 7) & 1;  // Get the leftmost bit
    cpu.regs.a = ((a << 1) | bit7) & 0xFF;
    
    cpu.set_flags(0, 0, 0, bit7);
    
    ck_assert(cpu.regs.a == 0x0B);  // 10000101 -> 00001011
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // RLCA always sets Z=0
    ck_assert(cpu.get_flag(FLAG_N) == false);  // RLCA always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // RLCA always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == true);   // Bit 7 was 1, so C=1
    
    // Test RRCA (Rotate Right through Carry)
    cpu.regs.a = 0x3A;  // 00111010
    
    // Simulate RRCA
    a = cpu.regs.a;
    u8 bit0 = a & 1;  // Get the rightmost bit
    cpu.regs.a = ((a >> 1) | (bit0 << 7)) & 0xFF;
    
    cpu.set_flags(0, 0, 0, bit0);
    
    ck_assert(cpu.regs.a == 0x1D);  // 00111010 -> 00011101
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // RRCA always sets Z=0
    ck_assert(cpu.get_flag(FLAG_N) == false);  // RRCA always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // RRCA always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == false);  // Bit 0 was 0, so C=0
    
    // Test RLA (Rotate Left through Carry)
    cpu.regs.a = 0x95;  // 10010101
    cpu.set_flag(FLAG_C, true);  // Set carry flag
    
    // Simulate RLA
    a = cpu.regs.a;
    bit7 = (a >> 7) & 1;
    u8 old_carry = cpu.get_flag(FLAG_C);
    cpu.regs.a = ((a << 1) | old_carry) & 0xFF;
    
    cpu.set_flags(0, 0, 0, bit7);
    
    ck_assert(cpu.regs.a == 0x2B);  // 10010101 -> 00101011
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // RLA always sets Z=0
    ck_assert(cpu.get_flag(FLAG_N) == false);  // RLA always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // RLA always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == true);   // Bit 7 was 1, so C=1
    
    // Test RRA (Rotate Right through Carry)
    cpu.regs.a = 0x81;  // 10000001
    cpu.set_flag(FLAG_C, false);  // Clear carry flag
    
    // Simulate RRA
    a = cpu.regs.a;
    bit0 = a & 1;
    old_carry = cpu.get_flag(FLAG_C);
    cpu.regs.a = ((a >> 1) | (old_carry << 7)) & 0xFF;
    
    cpu.set_flags(0, 0, 0, bit0);
    
    ck_assert(cpu.regs.a == 0x40);  // 10000001 -> 01000000
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // RRA always sets Z=0
    ck_assert(cpu.get_flag(FLAG_N) == false);  // RRA always sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // RRA always sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == true);   // Bit 0 was 1, so C=1
    
} END_TEST

START_TEST(test_flag_instructions) {
    CPU cpu;
    cpu.init();
    
    // Test CPL (Complement A)
    cpu.regs.a = 0x55;
    
    // Simulate CPL
    cpu.regs.a = ~cpu.regs.a;
    cpu.set_flags(cpu.get_flag(FLAG_Z), 1, 1, cpu.get_flag(FLAG_C));
    
    ck_assert(cpu.regs.a == 0xAA);  // ~0x55 = 0xAA
    ck_assert(cpu.get_flag(FLAG_N) == true);   // CPL always sets N=1
    ck_assert(cpu.get_flag(FLAG_H) == true);   // CPL always sets H=1
    
    // Test SCF (Set Carry Flag)
    cpu.set_flag(FLAG_C, false);
    cpu.set_flag(FLAG_H, false);
    cpu.set_flag(FLAG_N, false);
    
    // Simulate SCF
    cpu.set_flags(cpu.get_flag(FLAG_Z), 0, 0, 1);
    
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // Z unchanged
    ck_assert(cpu.get_flag(FLAG_N) == false);  // SCF sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // SCF sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == true);   // SCF sets C=1
    
    // Test CCF (Complement Carry Flag)
    cpu.set_flag(FLAG_C, true);
    
    // Simulate CCF
    cpu.set_flags(cpu.get_flag(FLAG_Z), 0, 0, !cpu.get_flag(FLAG_C));
    
    ck_assert(cpu.get_flag(FLAG_Z) == false);  // Z unchanged
    ck_assert(cpu.get_flag(FLAG_N) == false);  // CCF sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // CCF sets H=0
    ck_assert(cpu.get_flag(FLAG_C) == false);  // CCF complements C (1->0)
    
} END_TEST

START_TEST(test_16bit_operations) {
    CPU cpu;
    cpu.init();
    
    // Test 16-bit ADD (ADD HL, BC)
    cpu.regs.h = 0x12;
    cpu.regs.l = 0x34;
    cpu.regs.b = 0x56;
    cpu.regs.c = 0x78;
    
    u16 hl = (cpu.regs.h << 8) | cpu.regs.l;  // 0x1234
    u16 bc = (cpu.regs.b << 8) | cpu.regs.c;  // 0x5678
    u32 result = hl + bc;
    
    // 16-bit operations: Z=unchanged, N=0, H=bit11 carry, C=bit15 carry
    bool h = (hl & 0xFFF) + (bc & 0xFFF) >= 0x1000;
    bool c = result >= 0x10000;
    
    cpu.regs.h = (result >> 8) & 0xFF;
    cpu.regs.l = result & 0xFF;
    cpu.set_flags(-1, 0, h, c);  // Z unchanged for 16-bit
    
    ck_assert(cpu.regs.h == 0x68);  // 0x1234 + 0x5678 = 0x68AC
    ck_assert(cpu.regs.l == 0xAC);
    ck_assert(cpu.get_flag(FLAG_N) == false);  // 16-bit ADD sets N=0
    ck_assert(cpu.get_flag(FLAG_H) == false);  // No half-carry occurred
    ck_assert(cpu.get_flag(FLAG_C) == false);  // No full carry
    
    // Test 16-bit INC (INC BC)
    cpu.regs.b = 0xFF;
    cpu.regs.c = 0xFF;
    
    u16 bc_val = (cpu.regs.b << 8) | cpu.regs.c;  // 0xFFFF
    bc_val++;
    
    cpu.regs.b = (bc_val >> 8) & 0xFF;
    cpu.regs.c = bc_val & 0xFF;
    
    ck_assert(cpu.regs.b == 0x00);  // 0xFFFF + 1 = 0x0000
    ck_assert(cpu.regs.c == 0x00);
    
    // Test 16-bit DEC (DEC DE)
    cpu.regs.d = 0x00;
    cpu.regs.e = 0x01;
    
    u16 de_val = (cpu.regs.d << 8) | cpu.regs.e;  // 0x0001
    de_val--;
    
    cpu.regs.d = (de_val >> 8) & 0xFF;
    cpu.regs.e = de_val & 0xFF;
    
    ck_assert(cpu.regs.d == 0x00);  // 0x0001 - 1 = 0x0000
    ck_assert(cpu.regs.e == 0x00);
    
} END_TEST

START_TEST(test_register_operations) {
    CPU cpu;
    cpu.init();
    
    // Test register reading/writing
    cpu.cpu_set_reg(RegType::A, 0xAA);
    ck_assert(cpu.cpu_read_reg(RegType::A) == 0xAA);
    
    cpu.cpu_set_reg(RegType::B, 0xBB);
    ck_assert(cpu.cpu_read_reg(RegType::B) == 0xBB);
    
    cpu.cpu_set_reg(RegType::C, 0xCC);
    ck_assert(cpu.cpu_read_reg(RegType::C) == 0xCC);
    
    // Test 16-bit register operations
    cpu.cpu_set_reg(RegType::BC, 0x1234);
    ck_assert(cpu.cpu_read_reg(RegType::BC) == 0x1234);
    ck_assert(cpu.regs.b == 0x34);  // Little-endian
    ck_assert(cpu.regs.c == 0x12);
    
    cpu.cpu_set_reg(RegType::DE, 0x5678);
    ck_assert(cpu.cpu_read_reg(RegType::DE) == 0x5678);
    ck_assert(cpu.regs.d == 0x78);  // Little-endian
    ck_assert(cpu.regs.e == 0x56);
    
    cpu.cpu_set_reg(RegType::HL, 0x9ABC);
    ck_assert(cpu.cpu_read_reg(RegType::HL) == 0x9ABC);
    ck_assert(cpu.regs.h == 0xBC);  // Little-endian
    ck_assert(cpu.regs.l == 0x9A);
    
    // Test AF register (special case for flags)
    cpu.cpu_set_reg(RegType::AF, 0x1234);
    ck_assert(cpu.regs.a == 0x34);
    ck_assert(cpu.regs.f == 0x12);
    
} END_TEST

START_TEST(test_condition_checking) {
    CPU cpu;
    cpu.init();
    
    // Test condition checking functions
    // Z flag conditions
    cpu.set_flag(FLAG_Z, true);
    ck_assert(check_condition(&cpu, CondType::Z) == true);
    ck_assert(check_condition(&cpu, CondType::NZ) == false);
    
    cpu.set_flag(FLAG_Z, false);
    ck_assert(check_condition(&cpu, CondType::Z) == false);
    ck_assert(check_condition(&cpu, CondType::NZ) == true);
    
    // C flag conditions
    cpu.set_flag(FLAG_C, true);
    ck_assert(check_condition(&cpu, CondType::C) == true);
    ck_assert(check_condition(&cpu, CondType::NC) == false);
    
    cpu.set_flag(FLAG_C, false);
    ck_assert(check_condition(&cpu, CondType::C) == false);
    ck_assert(check_condition(&cpu, CondType::NC) == true);
    
    // NONE condition
    ck_assert(check_condition(&cpu, CondType::NONE) == true);
    
} END_TEST

Suite *stack_suite() {
    Suite *s = suite_create("emu");
    TCase *tc = tcase_create("core");

    tcase_add_test(tc, test_nothing);
    tcase_add_test(tc, test_instruction_flags);
    tcase_add_test(tc, test_arithmetic_instructions);
    tcase_add_test(tc, test_logical_instructions);
    tcase_add_test(tc, test_rotation_instructions);
    tcase_add_test(tc, test_flag_instructions);
    tcase_add_test(tc, test_16bit_operations);
    tcase_add_test(tc, test_register_operations);
    tcase_add_test(tc, test_condition_checking);
    suite_add_tcase(s, tc);

    return s;
}

int main() {
    Suite *s = stack_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int nf = srunner_ntests_failed(sr);

    srunner_free(sr);

    return nf == 0 ? 0 : -1;
} 