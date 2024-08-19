// VMTutorial.cpp : This file contains the main part of the program.
//
// This program is a virutal machine implimentation of the Little Computer 3 cpu.
// This program is also capable of running lc3 files downloaded from the internet.

#include "utils.hpp"

#include <csignal>
#include <iostream>

int main(int argc, char ** argv)
{
    std::ios::sync_with_stdio(false);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    if (argc < 2) {
        std::cerr << "Usage: VMTutorial <image-file>" << std::endl;
        return 1;
    }

    std::signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    const char * filename = argv[1];
    if (!open_image(filename)) {
        std::cerr << "Error: Failed to open image file \"" << std::endl;
        return 2;
    }

    reg[R_COND] = FL_ZER;

    int PSTART = 0x3000;
    reg[R_PC] = PSTART;

    bool running = true;
    while (running) {
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;

        switch (op) {
        case OP_ADD:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            bool imm_mode = instr & 0x20;

            if (imm_mode) {
                uint16_t imm5 = sign_extend(instr & 0x1f, 5);
                reg[r0] = reg[r1] + imm5;
            }
            else {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] + reg[r2];
            }

            update_flags(r0);
        }
            break;
        case OP_AND:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            bool imm_mode = instr & 0x20;
            if (imm_mode) {
                uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                reg[r0] = reg[r1] & imm5;
            }
            else {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] & reg[r2];
            }
            update_flags(r0);
        }
            break;
        case OP_BR:
        {
            uint16_t cond_flag = (instr >> 9) & 0x7;
            if (cond_flag & reg[R_COND]) {
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                reg[R_PC] += pc_offset;
            }
        }
            break;
        case OP_JMP:
        {
            uint16_t r0 = (instr >> 6) & 0x7;
            reg[R_PC] = reg[r0];
        }
            break;
        case OP_JSR:
        {
            reg[R_R7] = reg[R_PC];
            bool imm_mode = instr & 0x800;
            if (imm_mode) {
                uint16_t pc_offset = sign_extend(instr & 0x7FF, 11);
                reg[R_PC] += pc_offset;
            }
            else {
                uint16_t r0 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r0];
            }
        }
            break;
        case OP_LD:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            reg[r0] = mem_read(reg[R_PC] + pc_offset);
            update_flags(r0);
        }
            break;
        case OP_LDI:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
            update_flags(r0);
        }
            break;
        case OP_LDR:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t r1_offset = sign_extend(instr & 0x3F, 6);
            reg[r0] = mem_read(reg[r1] + r1_offset);
            update_flags(r0);
        }
            break;
        case OP_LEA:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            reg[r0] = reg[R_PC] + pc_offset;
            update_flags(r0);
        }
            break;
        case OP_NOT:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            reg[r0] = ~reg[r1];
            update_flags(r0);
        }
            break;
        case OP_ST:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            mem_write(reg[R_PC] + pc_offset, reg[r0]);
        }
            break;
        case OP_STI:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
        }
            break;
        case OP_STR:
        {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t r1_offset = sign_extend(instr & 0x3F, 6);
            mem_write(reg[r1] + r1_offset, reg[r0]);
        }
            break;
        case OP_TRAP:
            reg[R_R7] = reg[R_PC];
            switch (instr & 0xFF) {
            case TRAP_GETC:
            {
                reg[R_R0] = (uint16_t)getchar();
                update_flags(R_R0);
            }
                break;
            case TRAP_PUTS:
            {
                uint16_t* ptr = memory + reg[R_R0];
                while (*ptr) {
                    std::cout << (char)(*ptr);
                    ptr++;
                }
                std::cout << std::flush;
            }
                break;
            case TRAP_PUTSP:
            {
                uint16_t* ptr = memory + reg[R_R0];
                while (*ptr) {
                    char c1 = *ptr & 0xFF;
                    std::cout << c1;
                    char c2 = (*ptr) >> 8;
                    if (c2) std::cout << c2;
                    ptr++;
                }
                std::cout << std::flush;
            }
                break;
            case TRAP_IN:
            {
                std::cout << "Enter a character: ";
                reg[R_R0] = (uint16_t)getchar();
                update_flags(R_R0);
            }
                break;
            case TRAP_OUT:
            {
                char c = (char)reg[R_R0];
                std::cout << c << std::flush;
            }
                break;
            case TRAP_HALT:
            {
                std::cout << "Program Quit" << std::endl;
                running = false;
            }
                break;
            default:
                abort();
                break;
            }
            break;
        default:
            abort();
            break;
        }
    }

    restore_input_buffering();
}