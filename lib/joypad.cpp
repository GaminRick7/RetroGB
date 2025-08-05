#include "joypad.hpp"
#include <SDL.h>

Joypad::Joypad() : button_state(0xFF), select_buttons(0), select_dpad(0) {
    // Initialize with all buttons released (active low, so 0xFF means all released)
}

u8 Joypad::read_joypad() {
    u8 result = 0xFF; // Default to all buttons released
    
    if (!select_buttons) {
        // Action buttons selected (A, B, Start, Select)
        result &= button_state & 0x0F;
    }
    
    if (!select_dpad) {
        // Directional buttons selected (Up, Down, Left, Right)
        result &= (button_state >> 4) & 0x0F;
    }
    
    // If neither buttons nor d-pad is selected, return 0xFF
    if (select_buttons && select_dpad) {
        return 0xFF;
    }
    
    return result;
}

void Joypad::write_joypad(u8 value) {
    // Bits 5 and 4 control which button group is selected
    select_buttons = (value >> 5) & 0x01;
    select_dpad = (value >> 4) & 0x01;
}

void Joypad::set_button_state(u8 button, bool pressed) {
    if (pressed) {
        button_state &= ~button; // Clear bit (active low)
    } else {
        button_state |= button;   // Set bit (released)
    }
}

void Joypad::update_from_sdl_key(int sdl_key, bool pressed) {
    switch (sdl_key) {
        case SDLK_a:        // A button
            set_button_state(BUTTON_A, pressed);
            break;
        case SDLK_s:        // B button
            set_button_state(BUTTON_B, pressed);
            break;
        case SDLK_RETURN:   // Start button
            set_button_state(BUTTON_START, pressed);
            break;
        case SDLK_SPACE:    // Select button
            set_button_state(BUTTON_SELECT, pressed);
            break;
        case SDLK_UP:       // Up arrow
            set_button_state(BUTTON_UP << 4, pressed);
            break;
        case SDLK_DOWN:     // Down arrow
            set_button_state(BUTTON_DOWN << 4, pressed);
            break;
        case SDLK_LEFT:     // Left arrow
            set_button_state(BUTTON_LEFT << 4, pressed);
            break;
        case SDLK_RIGHT:    // Right arrow
            set_button_state(BUTTON_RIGHT << 4, pressed);
            break;
    }
} 