#pragma once

#include "common.hpp"

class RomHeader {
public:
    u8 entry[4];
    u8 logo[0x30];
    char title[16];
    u16 new_lic_code;
    u8 sgb_flag;
    u8 type;
    u8 rom_size;
    u8 ram_size;
    u8 dest_code;
    u8 lic_code;
    u8 version;
    u8 checksum;
    u16 global_checksum;
};

class Cartridge {
private:
    char filename[1024];
    u32 rom_size;
    u8* rom_data;
    RomHeader* header;

    static const char* ROM_TYPES[];
    static const char* LIC_CODE[];

public:
    Cartridge();
    ~Cartridge();
    
    bool load(const char* cart);
    const char* get_lic_name() const;
    const char* get_type_name() const;

    u8 read(u16 address);
    void write(u16 address, u8 value);
}; 