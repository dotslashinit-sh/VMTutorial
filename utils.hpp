#pragma once

#include <Windows.h>
#include <conio.h>
#include <fstream>
#include <iostream>

HANDLE hStdin = INVALID_HANDLE_VALUE;

DWORD dwOldMode;

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX];

// Registers
enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, /* program counter */
    R_COND, /* conditional flag */
    R_COUNT
};

uint16_t reg[R_COUNT];

// OP codes
enum {
    OP_BR = 0, /* branch*/
    OP_ADD,    /* add */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

// Conditional flags
enum {
    FL_POS = 1, /* positive */
    FL_ZER = 2, /* zero */
    FL_NEG = 4  /* negative */
};

// Trap codes
enum {
    TRAP_GETC = 0x20,  /* gets character from keyboard (not echoed) */
    TRAP_OUT = 0x21,   /* outputs a character to console*/
    TRAP_PUTS = 0x22,  /* outputs a string to console */
    TRAP_IN = 0x23,    /* gets a character from keyboard (echoed) */
    TRAP_PUTSP = 0x24, /* output a string to console */
    TRAP_HALT = 0x25,  /* exit the program */
};

enum {
    MR_KBST = 0xFE00, // Keyboard status
    MR_KBDT = 0xFE02 // Keyboard data
};

// Performs sign-extension to n-sized integer and converts it into a 16-bit signed integer.
uint16_t sign_extend(uint16_t value, const size_t bits) {
    if (value >> (bits - 1)) {
        value |= (0xFFFF << bits);
    }
    return value;
}

void update_flags(uint16_t r) {
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZER;
    }
    else if (reg[r] & 0x8000) {
        reg[R_COND] = FL_NEG;
    }
    else {
        reg[R_COND] = FL_POS;
    }
}

#define swap_e16(v) ((v << 8) | (v >> 8))

bool _read_file_to_memory(std::ifstream & file) {
    uint16_t origin = 0;
    file.read(reinterpret_cast<char*>(&origin), 2);
    origin = swap_e16(origin);

    uint16_t max = MEMORY_MAX - origin;
    auto ptr = memory + origin;
    size_t read = file.readsome(reinterpret_cast<char*>(ptr), max);
    read = read / sizeof(uint16_t);

    // Swap to little endian
    while (read-- > 0) {
        *ptr = swap_e16(*ptr);
        ptr++;
    }
    return true;

}

bool open_image(const char * filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Error opening image file!" << std::endl;
        return false;
    }
    _read_file_to_memory(file);
    file.close();
    return true;
}

void disable_input_buffering() {
    GetConsoleMode(hStdin, &dwOldMode);
    DWORD dwMode = dwOldMode ^ ENABLE_ECHO_INPUT ^ ENABLE_LINE_INPUT;
    SetConsoleMode(hStdin, dwMode);
    FlushConsoleInputBuffer(hStdin);
}

void restore_input_buffering() {
    SetConsoleMode(hStdin, dwOldMode);
}

bool check_key() {
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}

void handle_interrupt(int signal) {
    std::cerr << "Interrupt received!Closing vm...\n" << std::endl;
    exit(-2);
}

uint16_t mem_read(uint16_t address) {
    if (address == MR_KBST) {
        if (check_key()) {
            memory[MR_KBST] = (1 << 15);
            memory[MR_KBDT] = (uint16_t)getchar();
        }
        else {
            memory[MR_KBST] = 0;
        }
    }
    return memory[address];
}

void mem_write(uint16_t address, uint16_t value) {
    memory[address] = value;
}