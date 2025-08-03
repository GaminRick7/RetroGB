#include "timer.hpp"
#include "cpu.hpp"

void Timer::init() {
    // Initialize timer registers and state
    div = 0xAC00;
}

void Timer::tick() {
    // Implement timer tick logic
    u16 prev_div = div;
    div++;

    bool timer_update = false;

    switch (tac & 0b11) {
        case 0b00:
            timer_update = (prev_div & (1 << 9)) && !(div & (1 << 9));
            break;
        case 0b01:
            timer_update = (prev_div & (1 << 3)) && !(div & (1 << 3));
            break;
        case 0b10:
            timer_update = (prev_div & (1 << 5)) && !(div & (1 << 5));
            break;
        case 0b11:
            timer_update = (prev_div & (1 << 7)) && !(div & (1 << 7));
            break;
    }

    if (timer_update && tac & (1 << 2)) {
        tima++;
        if (tima == 0xFF) {
            tima = tma;
            cpu->request_interrupt(IT_TIMER);
        }
    }
}

void Timer::write(u16 address, u8 value) {
    switch (address) {
        case 0xFF04:
            div = 0;
            break;
        case 0xFF05:
            tima = value;
            break;
        case 0xFF06:
            tma = value;
            break;
        case 0xFF07:
            tac = value;
            break;
    }
    return;
}

u8 Timer::read(u16 address) {
    switch (address) {
        case 0xFF04:
            return div >> 8;
        case 0xFF05:
            return tima;
        case 0xFF06:
            return tma;
        case 0xFF07:
            return tac;
    }
    return 0;
}