#pragma once

#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

#define BIT(a, n) ((a & (1 << n)) ? 1 : 0)

#define BIT_SET(a, n, on) (on ? (a) |= (1 << n) : (a) &= ~(1 << n))

#define BETWEEN(a, b, c) ((a >= b) && (a <= c))

constexpr uint8_t FLAG_Z = 1 << 7; // Zero
constexpr uint8_t FLAG_N = 1 << 6; // Subtract
constexpr uint8_t FLAG_H = 1 << 5; // Half-Carry
constexpr uint8_t FLAG_C = 1 << 4; // Carry

// Interrupt types
constexpr uint8_t IT_VBLANK = 1 << 0;
constexpr uint8_t IT_LCD_STAT = 1 << 1;
constexpr uint8_t IT_TIMER = 1 << 2;
constexpr uint8_t IT_SERIAL = 1 << 3;
constexpr uint8_t IT_JOYPAD = 1 << 4;

void delay(u32 ms); 