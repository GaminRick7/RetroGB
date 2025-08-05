#pragma once

#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>

// ===== TYPE DEFINITIONS =====
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

// ===== BIT MANIPULATION MACROS =====
#define BIT(a, n) ((a & (1 << n)) ? 1 : 0)
#define BIT_SET(a, n, on) (on ? (a) |= (1 << n) : (a) &= ~(1 << n))
#define BETWEEN(a, b, c) ((a >= b) && (a <= c))

// ===== DEBUG CONFIGURATION =====
// Debug mode flag - set to 1 to enable debug features, 0 to disable
#define DEBUG_MODE 0

// ===== CPU FLAGS =====
constexpr uint8_t FLAG_Z = 1 << 7; // Zero flag
constexpr uint8_t FLAG_N = 1 << 6; // Subtract flag
constexpr uint8_t FLAG_H = 1 << 5; // Half-carry flag
constexpr uint8_t FLAG_C = 1 << 4; // Carry flag

// ===== INTERRUPT TYPES =====
constexpr uint8_t IT_VBLANK = 1 << 0;    // V-Blank interrupt
constexpr uint8_t IT_LCD_STAT = 1 << 1;  // LCD Status interrupt
constexpr uint8_t IT_TIMER = 1 << 2;     // Timer interrupt
constexpr uint8_t IT_SERIAL = 1 << 3;    // Serial interrupt
constexpr uint8_t IT_JOYPAD = 1 << 4;    // Joypad interrupt

// ===== UTILITY FUNCTIONS =====
/**
 * @brief Delay execution for specified milliseconds
 * @param ms Number of milliseconds to delay
 */
void delay(u32 ms); 