#ifndef __EEPROM_EMU__
#define __EEPROM_EMU__


#include <stdint.h>

/* Driver
class FlashDriver {
public:
    enum { BankSize = XXXX };
    static void erase(uint8_t bank) {};

    static uint32_t read(uint8_t bank, uint16_t addr);

    // If 32bit write requires multiple operations, those MUST follow from
    // least significant bits to most significant bits.
    static uint32_t write(uint8_t bank, uint16_t addr, uint32_t data);
}
*/

/*
    Bank structure:

    [ 4 bytes bank marker ] [ 6 bytes record ] [ 6 bytes record ]...

    Marker: 0x00000000 - bank used (current). 0xFFFFFFFF - bank free

    Data record: [ 3bit commit flags, 13bit address, 32 bits value ]

    Flags:

    111 xxxxx xxxxxxxx - free to use
    011 xxxxx xxxxxxxx - write in progress
    001 xxxxx xxxxxxxx - commited (valid)
    000 xxxxx xxxxxxxx - stale
*/

template <typename FLASH_DRIVER>
class EepromEmu
{
    static const uint16_t STATE_BANK_CLEAR  = 0xFFFF;
    static const uint16_t STATE_BANK_ACTIVE = 0x7FFF;
    static const uint16_t STATE_BANK_FULL   = 0x3FFF;

    bool initialized = false;
    uint8_t current_bank = 0;
    uint32_t next_write_offset[2]; // Pointers to free bank areas

    bool is_clear(uint8_t bank)
    {
        for (uint32_t i = 0; i < FLASH_DRIVER::BankSize / 4; i++) {
            if (flash.read_u16(bank, i*4) != 0xFFFFFFFF) return false;
        }
        return true;
    }

    uint32_t find_write_offset()
    {
        uint32_t ofs = 4;

        for (; ofs <= FLASH_DRIVER::BankSize - 6; ofs += 6)
        {
            if ((flash.read_u16(current_bank, ofs) == 0xFFFF) &&
                (flash.read_u16(current_bank, ofs + 2) == 0xFFFF) &&
                (flash.read_u16(current_bank, ofs + 4) == 0xFFFF)) break;
        }

        return ofs;
    }

    void move_bank(uint8_t from, uint8_t to, uint16_t ignore_addr=UINT16_MAX)
    {
        if (!is_clear(to)) flash.erase(to);

        current_bank = to;
        next_write_offset[to] = 4;

        // Scan & copy values
        for (uint32_t ofs = 4; ofs <= FLASH_DRIVER::BankSize - 6; ofs += 6)
        {
            uint16_t head = flash.read_u16(from, ofs);
            uint16_t addr = head & 0x1FFF;

            // End reached => Break
            if ((head & 0xE000) == 0xE000) break;
            // Not commit => Skip
            if ((head & 0xE000) != 0x2000) continue;
            // Skip variable with ignored address
            if (addr == ignore_addr) continue;


            uint16_t lo = flash.read_u16(from, ofs + 2);
            uint16_t hi = flash.read_u16(from, ofs + 4);
            uint32_t val = (hi << 16) | lo;

            // Scan next records and check for more fresh
            for (uint32_t final_ofs = ofs + 6; final_ofs <= FLASH_DRIVER::BankSize - 6; final_ofs += 6)
            {
                uint16_t new_head = flash.read_u16(from, final_ofs);

                // End reached => nothing to search
                if ((new_head & 0xE000) == 0xE000) break;
                // Not commit => skip
                if ((new_head & 0xE000) != 0x2000) continue;
                // Different address => skip
                if ((new_head & 0x1FFF) != addr) continue;

                // More fresh value found => update
                lo = flash.read_u16(from, ofs + 2);
                hi = flash.read_u16(from, ofs + 4);
                val = (hi << 16) | lo;
            }

            // Write found variable to new bank
            write_bank_u32(to, addr, val);
        }

        // Mark new bank active and erase previous
        flash.write_u16(to, 0, STATE_BANK_ACTIVE);
        flash.erase(from);
    }

    void init()
    {
        initialized = true;

        uint16_t state0 = flash.read_u16(0, 0);
        uint16_t state1 = flash.read_u16(1, 0);

        switch (state0)
        {
        case STATE_BANK_ACTIVE:
            current_bank = 0;
            next_write_offset[0] = find_write_offset();
            if (state1 != STATE_BANK_CLEAR) flash.erase(current_bank ^ 1);
            return;

        case STATE_BANK_FULL:
            move_bank(0, 1);
            return;

        case STATE_BANK_CLEAR:
            switch (state1)
            {
            case STATE_BANK_CLEAR:
                // Both clear, format 0 & select it

                // Make sure all block clear
                if (!is_clear(0)) flash.erase(0);

                flash.write_u16(0, 0, STATE_BANK_ACTIVE);
                current_bank = 0;
                next_write_offset[0] = find_write_offset();
                return;

            case STATE_BANK_ACTIVE:
                current_bank = 1;
                next_write_offset[1] = find_write_offset();
                return;

            case STATE_BANK_FULL:
                move_bank(1, 0);
                return;

            default:
                // Broken bank 1 data
                flash.erase(1);
                init();
                return;
            }
            return;

        default:
            // Broken bank 0 data
            flash.erase(0);
            init();
            return;
        }
    }

    uint32_t read_bank_u32(uint8_t bank, uint16_t addr, uint32_t dflt)
    {
        if (!initialized) init();

        uint32_t result = dflt;

        for (uint32_t ofs = 4; ofs <= FLASH_DRIVER::BankSize - 6; ofs += 6)
        {
            uint16_t head = flash.read_u16(bank, ofs);
            uint16_t lo = flash.read_u16(bank, ofs + 2);
            uint16_t hi = flash.read_u16(bank, ofs + 4);

            // Reached free space => stop
            if (head == 0xFFFF && lo == 0xFFFF && hi == 0xFFFF) break;

            // If marker flags are 001 xxxxx xxxxxxxxx => data valid,
            // update result if address match
            if ((head & (0xE000)) != 0x2000) continue;
            if ((head & 0x1FFF) != addr) continue;

            result = (hi << 16) + lo;
        }

        return result;
    }

    void write_bank_u32(uint8_t bank, uint16_t addr, uint32_t val)
    {
        if (!initialized) init();

        uint32_t previous = read_bank_u32(bank, addr, val+1);
        if (previous == val) return;

        // Check free space and swap banks if needed
        if (next_write_offset[bank] + 6 > FLASH_DRIVER::BankSize)
        {
            move_bank(bank, bank ^ 1, addr);
            bank ^= 1;
        }

        // Reset first flag, mark recored occupied
        flash.write_u16(bank, next_write_offset[bank], 0x7FFF);

        // Write data
        flash.write_u16(bank, next_write_offset[bank] + 2, val & 0xFFFF);
        flash.write_u16(bank, next_write_offset[bank] + 4, (uint16_t)(val >> 16) & 0xFFFF);

        // Write address (with existing flags state '011x xxxx xxxx xxxx')
        flash.write_u16(bank, next_write_offset[bank], addr | 0x6000);

        // Write second commit flag '001x xxxx xxxx xxxx'
        flash.write_u16(bank, next_write_offset[bank], addr | 0x2000);

        next_write_offset[bank] += 6;

        // Search previous records and mark those stale
        for (uint32_t ofs = 4; ofs < next_write_offset[bank]-6; ofs += 6)
        {
            if ((flash.read_u16(bank, ofs) & 0x1FFF) == addr)
            {
                flash.write_u16(bank, ofs, 0);
            }
        }
    }

public:
    FLASH_DRIVER flash;

    uint32_t read_u32(uint16_t addr, uint32_t dflt)
    {
        return read_bank_u32(current_bank, addr, dflt);
    }

    void write_u32(uint16_t addr, uint32_t val)
    {
        write_bank_u32(current_bank, addr, val);
    }

    float read_float(uint16_t addr, float dflt)
    {
        union { uint32_t i; float f; } x;
        x.f = dflt;
        x.i = read_u32(addr, x.i);
        return x.f;
    }

    void write_float(uint16_t addr, float val)
    {
        union { uint32_t i; float f; } x;
        x.f = val;
        write_u32(addr, x.i);
    }
};

#endif
