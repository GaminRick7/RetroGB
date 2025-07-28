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

void delay(u32 ms); 