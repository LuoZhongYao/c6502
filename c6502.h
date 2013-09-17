#ifndef __C6502_H__
#define __C6502_H__
#include    <stdint.h>

typedef union{
    uint8_t value;
    struct{
        uint16_t valid:7;
        uint16_t s1:1;
        uint16_t s2:1;
    };
    uint16_t slue;
}Register;

typedef struct{
    Register    A;
    Register    X;
    Register    Y;
    Register    S;
    Register    PC;
    union{
        struct{
            uint16_t C:1;       /*! 进位 !*/
            uint16_t Z:1;       /*! 零标志 !*/
            uint16_t I:1;       /*! 中断禁止 !*/
            uint16_t D:1;       /*! 十进制运算标志 !*/
            uint16_t B:1;       /*! 中断标志 !*/
            uint16_t  :1;
            uint16_t V:1;       /*! 溢出标志 !*/
            uint16_t N:1;       /*! 负数标志 !*/
        };
        Register P;
    };
}CPU;

typedef union{
    struct{
        uint8_t  page0[0x100];
        uint8_t  stack[0x100];
        uint8_t  card[0x100];
        uint8_t  soft[0x500];
        uint8_t  held[0x1800];
        uint8_t  ppuIO[0x2000];
        uint8_t  cpuIO[0x2000];
        uint8_t  user[0x2000];
        uint8_t  softRom[0x8000];
    };
    uint8_t ram[64 * 1024];
}CPURAM;

extern void runCpu(void (*fn)(void),void (*err)(void));
extern void powerOn(void);
extern void powerOff(void);
extern void loadRom(const char *rom);
extern void printCpu(void);
extern void input(char ch);
extern void output(char ch);
extern void disassembly(uint16_t,uint16_t);

#endif
