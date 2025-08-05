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

    bool ram_enabled;
    bool ram_banking;

    u8 *rom_bank_x;
    u8 banking_mode;

    u8 rom_bank_value;
    u8 ram_bank_value;

    u8 *ram_bank; //current selected ram bank
    u8 *ram_banks[16]; //all ram banks

    //for battery
    bool battery; //has battery
    bool need_save; //should save battery backup

public:
    Cartridge();
    ~Cartridge();
    
    bool load(const char* cart);
    const char* get_lic_name() const;
    const char* get_type_name() const; 

    bool is_mbc1();
    void setup_banking();
    bool get_need_save();
    void battery_load();
    void battery_save();

    u8 read(u16 address);
    void write(u16 address, u8 value);
}; 