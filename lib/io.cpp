#include "io.hpp"

u8 IO::io_read(u16 address){
    if (address == 0xFF00){
        return joypad->read_joypad();
    }
    else if (address == 0xFF01){
        return serial_data[0];
    }
    else if (address == 0xFF02){
        return serial_data[1];
    }
    else if (address >= 0xFF04 && address <= 0xFF07){
        return timer->read(address);
    }
    else if (address == 0xFF0F){
        return cpu->get_int_flags();
    }
    else if (address >= 0xFF40 && address <= 0xFF4B){
        return lcd->read(address);
    }

    return 0;
}

void IO::io_write(u16 address, u8 value){
    if (address == 0xFF00){
        joypad->write_joypad(value);
    }
    else if (address == 0xFF01){
        serial_data[0] = value;
    }
    else if (address >= 0xFF04 && address <= 0xFF07){
        timer->write(address, value);
    }
    else if (address == 0xFF02){
        serial_data[1] = value;
    }
    else if (address == 0xFF0F){
        cpu->set_int_flags(value);
    }
    else if (address >= 0xFF40 && address <= 0xFF4B){
        lcd->write(address, value);
    }
}