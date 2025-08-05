#pragma once

#include "common.hpp"

class Joypad {
private:
    // Joypad state - buttons are active low (0 = pressed, 1 = released)
    u8 button_state;      // Current state of all buttons
    u8 select_buttons;    // Select action buttons (A, B, Start, Select)
    u8 select_dpad;       // Select directional buttons (Up, Down, Left, Right)
    
    // Button bit positions (active low)
    static constexpr u8 BUTTON_A      = 0x01;
    static constexpr u8 BUTTON_B      = 0x02;
    static constexpr u8 BUTTON_SELECT = 0x04;
    static constexpr u8 BUTTON_START  = 0x08;
    static constexpr u8 BUTTON_RIGHT  = 0x01;
    static constexpr u8 BUTTON_LEFT   = 0x02;
    static constexpr u8 BUTTON_UP     = 0x04;
    static constexpr u8 BUTTON_DOWN   = 0x08;

public:
    Joypad();
    
    // Joypad register read/write
    u8 read_joypad();
    void write_joypad(u8 value);
    
    // Button state management
    void set_button_state(u8 button, bool pressed);
    void update_from_sdl_key(int sdl_key, bool pressed);
    
    // Get current button state for debugging
    u8 get_button_state() const { return button_state; }
}; 