#include    "c6502.h"
#include    <stdlib.h>
#include    <string.h>
#include    <stdio.h>
#include    <unistd.h>

static CPU cpu = {{0}};
static CPURAM *ram = NULL;

#define PC  (cpu.PC.slue)
#define A   (cpu.A)
#define X   (cpu.X)
#define Y   (cpu.Y)
#define S   (cpu.S)
#define P   (cpu.P)
#define C   (cpu.C)
#define Z   (cpu.Z)
#define I   (cpu.I)
#define D   (cpu.D)
#define B   (cpu.B)
#define V   (cpu.V)
#define N   (cpu.N)

#define RAM (ram->ram)
#define STACK   (ram->stack)

//static uint8_t STACK[0x100];

static void print_hex(uint8_t *data,size_t len,int index){
    int i = 0;
    for(i = 0;i < (index & 0xF);i++){
        if(i == 0) printf("\n%08X : ",index & (~0xF));
        else if(!(i & 7)) printf("\t");
        printf("   ");
    }
    for(i = 0;i < len;i++){
        if(!((i + index) & 0xf)) printf("\n%08X : ",i + index);
        else if(!((i + (index & 0xf)) & 7)) printf("\t");
        printf("%02X ",data[i]);
    }
    printf("\n");
}

void printCpu(void){
    printf("C = %1d  Z = %1d  I = %1d  D = %1d  B = %1d  V = %1d  N = %1d\n",C,Z,I,D,B,V,N);
    printf("A = %02hhX X = %02hhX Y = %02hhX S = %02hhx\n\e[31mPC = %04hX\e[0m %02X\n",A.value,X.value,Y.value,S.value,PC,RAM[PC]);
}

void powerOn(void){
    ram = calloc(1,sizeof(CPURAM));
    if(ram == NULL){
        printf("Power on : Memory Out!\n");
        exit(1);
    }
    PC = 0x817A;
    S.value = 0xFF;
}

void powerOff(void){
    if(ram) free(ram);
}


#define ADDR8(addr)     ((uint8_t*)(RAM + addr))
#define ADDR16(addr)    ((uint16_t *)(RAM + addr))

#define VALUE8(v)   (*((uint8_t*)(RAM + v)))
#define VALUE16(v)   (*((uint16_t*)(RAM + v)))

#define STACK8    (*((uint8_t *)(STACK + S.value)))
#define STACK16    (*((uint16_t *)(STACK + S.value)))

#define _LUI(v)       (VALUE8(v))                  /*! 立即数寻址 !*/
#define _ADDR(v)      (RAM[VALUE16(v)])            /*! 直接寻址 !*/
#define _ZADDR(v)     (RAM[VALUE8(v)])             /*! 零页寻址 !*/
#define _AADDR(r)     (r.value)                    /*! 隐含寻址 !*/
#define _ADDRX(v)     (RAM[VALUE16(v) + X.value])  /*! X变址寻址 !*/
#define _ADDRY(v)     (RAM[VALUE16(v) + Y.value])  /*! Y变址寻址 !*/
#define _ZADDRX(v)    (RAM[VALUE8(v) + X.value])   /*! 零页X变址 !*/
#define _ZADDRY(v)    (RAM[VALUE8(v) + Y.value])   /*! 零页Y变址 !*/
#define _XADDR(v)     (RAM[RAM[VALUE8(v) + X.value] | (RAM[VALUE8(v) + X.value + 1] << 8)])    /*! 先X变址，在间接寻址 !*/
#define _YADDR(v)     (RAM[((RAM[VALUE8(v)]) | (RAM[VALUE8(v) + 1] << 8)) + Y.value]) /*! 先间接寻址，在Y变址 !*/
#define _OFFSET(v)    ((int8_t)(VALUE8(v)))  /*! 相对寻址 !*/

#define OPERAND8    (VALUE8(PC + 1))
#define OPERAND16   (VALUE16(PC + 1))

#define addPC()   (PC += 1)
#define addPCOperand8() (PC += 2)
#define addPCOperand16()    (PC += 3)


#define LUI()       (_LUI(PC + 1))      /*! 立即数寻址 !*/
#define ADDR()      (_ADDR(PC + 1))     /*! 直接寻址 !*/
#define ZADDR()     (_ZADDR(PC + 1))    /*! 零页寻址 !*/
#define AADDR(r)    (r.value)           /*! 隐含寻址 !*/
#define ADDRX()     (_ADDRX(PC + 1))    /*! X变址寻址 !*/
#define ADDRY()     (_ADDRY(PC + 1))    /*! Y变址寻址 !*/
#define ZADDRX()    (_ZADDRX(PC + 1))   /*! 零页X变址 !*/
#define ZADDRY()    (_ZADDRY(PC + 1))   /*! 零页Y变址 !*/
#define XADDR()     (_XADDR(PC + 1))    /*! 先X变址，在间接寻址 !*/
#define YADDR()     (_YADDR(PC + 1))    /*! 先间接寻址，在Y变址 !*/
#define OFFSET()    (_OFFSET(PC + 1))   /*! 相对寻址 !*/


static inline int print_O(const char *opt,uint16_t addr){
    printf("%s \e[31m$%04hX\e[0m\t\t;",opt,addr + _OFFSET(addr));
    return 2;
}
static inline int print_XM(const char *opt,uint16_t addr){
    uint8_t x = X.value,v = VALUE8(addr);
    uint16_t index = (RAM[v + x]) |  (RAM[v + x + 1] << 8);
    uint8_t xv = RAM[index];
    printf("%s (\e[31m$%02hhX\e[0m,\e[34mX\e[0m)\t\t;($%02hhX,%02hhX) [$%04hX] = $%02hX\n",
            opt,v,v,x,index,xv);
    return 1;
}

static inline int print_MY(const char *opt,uint16_t addr){
    uint8_t y = Y.value,v = _LUI(addr);
    uint16_t index = ((RAM[v]) |  (RAM[v+ 1] << 8)) + y;
    uint8_t vy = RAM[index];
    printf("%s (\e[31m$%02hhX\e[0m),\e[34mY\e[0m\t\t;($%02hhX),%02hhX [$%04hX] = $%02hX\n",
            opt,v,v,y,index,vy);
    return 1;
}

static inline int print_L(const char *opt,uint16_t addr){
    printf("%s \e[31m#$%02hhX\e[0m\t\t;\n",opt,_LUI(addr));
    return 1;
}

static inline int print_A(const char *opt,uint16_t addr){
    printf("%s \e[31m$%04hX\e[0m\t\t;$%04hX = %02hhX\n",opt,VALUE16(addr),VALUE16(addr),_ADDR(addr));
    return 2;
} 

static inline int print_Z(const char *opt,uint16_t addr){
    printf("%s \e[31m$%02hhX\e[30m\t\t;$%04hX = %02hhX\n",opt,VALUE8(addr),VALUE16(addr),_ZADDR(addr));
    return 1;
}

static inline int print_R(const char *opt,uint16_t addr){
    printf("%s                  ;",opt);
    return 0;
}

static inline int print_X(const char *opt,uint16_t addr){
    printf("%s \e[31m$%04hX\e[0m,\e[34mX\e[0m\t\t;\t$%04hX = $%02hhX\n",opt,*ADDR16(addr),*ADDR16(addr) + X.value,_ADDRX(addr));
    return 2;
}

static inline int print_Y(const char *opt,uint16_t addr){
    printf("%s \e[31m$%04hX\e[0m,\e[34mY\e[0m\t\t;\t$%04hX = $%02hhX\n",opt,*ADDR16(addr),*ADDR16(addr) + Y.value,_ADDRX(addr));
    return 2;
}

static inline int print_M(const char *opt,uint16_t addr){
    printf("%s \e[31m$%04hX\e[0m,\e[34mY\e[0m\t\t;\t$%04hX = $%02hhX\n",opt,VALUE16(addr),VALUE16(addr),VALUE16(VALUE16(addr)));
    return 2;
}

static inline int  print_ZX(const char *opt,uint16_t addr){
    printf("%s \e[31m$%04hX\e[0m,\e[34mX\e[0m\t\t;\t$%02hX = $%02hhX\n",opt,*ADDR8(addr),*ADDR8(addr) + X.value,_ZADDRX(addr));
    return 1;
}

static inline int print_ZY(const char *opt,uint16_t addr){
    printf("%s \e[31m$%04hX\e[0m,\e[34mY\e[0m\t\t;\t$%02hX = $%02hhX\n",opt,*ADDR8(addr),*ADDR8(addr) + Y.value,_ZADDRX(addr));
    return 1;
}


static int disasm(uint16_t addr){
    printf("\e[33m%04hX\e[0m:\t%02hhX\t",addr,RAM[PC]);
    switch(RAM[addr]){
        default  : printf(" .byte  %02hhX\n",RAM[addr]); return 1;
        case 0x00: return printf("brk\n") + 1;
        case 0x01: return print_XM("ora",addr + 1) + 1;
        case 0x05: return print_Z("ora",addr + 1) + 1;
        case 0x06: return print_Z("asl",addr + 1) + 1;
        case 0x08: return printf ("php\n") + 1;
        case 0x09: return print_L("ora",addr + 1) + 1;
        case 0x0A: return print_R("asl",addr + 1) + 1;
        case 0x0D: return print_A("ora",addr + 1) + 1;
        case 0x0E: return print_A("asl",addr + 1) + 1;
        case 0x10: return printf ("bpl\n") + 1;
        case 0x11: return print_MY("ora",addr + 1) + 1;
        case 0x15: return print_ZX("ora",addr + 1) + 1;
        case 0x16: return print_ZX("asl",addr + 1) + 1;
        case 0x18: return printf ("clc\n") + 1;
        case 0x19: return print_Y("ora",addr + 1) + 1;
        case 0x1D: return print_X("ora",addr + 1) + 1;
        case 0x1E: return print_X("asl",addr + 1) + 1;
        case 0x20: return print_A("jsr",addr + 1) + 1;
        case 0x21: return print_XM("and",addr + 1) + 1;
        case 0x24: return print_Z("bit",addr + 1) + 1;
        case 0x25: return print_Z("and",addr + 1) + 1;
        case 0x26: return print_Z("rol",addr + 1) + 1;
        case 0x28: return printf("plp\n") + 1;
        case 0x29: return print_L("and",addr + 1) + 1;
        case 0x2A: return print_R("rol",addr + 1) + 1;
        case 0x2C: return print_A("bit",addr + 1) + 1;
        case 0x2D: return print_A("and",addr + 1) + 1; 
        case 0x2E: return print_A("rol",addr + 1) + 1;
        case 0x30: return printf("bmi\n") + 1;
        case 0x31: return print_MY("and",addr + 1) + 1;
        case 0x35: return print_ZX("and",addr + 1) + 1;
        case 0x36: return print_ZX("rol",addr + 1) + 1;
        case 0x38: return printf("sec\n") + 1;
        case 0x39: return print_Y("and",addr + 1) + 1;
        case 0x3D: return print_X("and",addr + 1) + 1;
        case 0x3E: return print_X("rol",addr + 1) + 1;
        case 0x40: return printf("rti\n") + 1;
        case 0x41: return print_XM("eor",addr + 1) + 1;
        case 0x45: return print_Z("eor",addr + 1) + 1;
        case 0x46: return print_Z("lsr",addr + 1) + 1;
        case 0x48: return printf("pha\n") + 1;
        case 0x49: return print_L("eor",addr + 1) + 1;
        case 0x4A: return print_R("lsr",addr + 1) + 1;
        case 0x4C: return print_A("jmp",addr + 1) + 1;
        case 0x4D: return print_A("eor",addr + 1) + 1;
        case 0x4E: return print_A("lsr",addr + 1) + 1;
        case 0x50: return printf("bvc\n") + 1;
        case 0x51: return print_MY("eor",addr + 1) + 1;
        case 0x55: return print_ZX("eor",addr + 1) + 1;
        case 0x56: return print_ZX("lsr",addr + 1) + 1;
        case 0x58: return printf("cli\n") + 1;
        case 0x59: return print_Y("eor",addr + 1) + 1;
        case 0x5D: return print_X("eor",addr + 1) + 1;
        case 0x5E: return print_X("lsr",addr + 1) + 1;
        case 0x60: return printf("rts\n") + 1;
        case 0x61: return print_XM("adc",addr + 1) + 1;
        case 0x65: return print_Z("adc",addr + 1) + 1;
        case 0x66: return print_Z("ror",addr + 1) + 1;
        case 0x68: return printf("pla\n") + 1;
        case 0x69: return print_L("adc",addr + 1) + 1;
        case 0x6A: return print_R("ror",addr + 1) + 1;
        case 0x6C: return print_M("jmp",addr + 1) + 1;
        case 0x6D: return print_A("adc",addr + 1) + 1;
        case 0x6E: return print_A("ror",addr + 1) + 1;
        case 0x70: return printf("bvs\n") + 1;
        case 0x71: return print_MY("adc",addr + 1) + 1;
        case 0x75: return print_ZX("adc",addr + 1) + 1;
        case 0x76: return print_ZX("ror",addr + 1) + 1;
        case 0x78: return printf("sei\n") + 1;
        case 0x79: return print_Y("adc",addr + 1) + 1;
        case 0x7D: return print_X("adc",addr + 1) + 1;
        case 0x7E: return print_X("ror",addr + 1) + 1;
        case 0x81: return print_XM("sta",addr + 1) + 1;
        case 0x84: return print_Z("sty",addr + 1) + 1;
        case 0x85: return print_Z("sta",addr + 1) + 1;
        case 0x86: return print_Z("stx",addr + 1) + 1;
        case 0x88: return printf("dey\n") + 1;
        case 0x8A: return printf("txa\n") + 1;
        case 0x8C: return print_A("sty",addr + 1) + 1;
        case 0x8D: return print_A("sta",addr + 1) + 1;
        case 0x8E: return print_A("stx",addr + 1) + 1;
        case 0x90: return printf("bcc\n") + 1;
        case 0x91: return print_MY("sta",addr + 1) + 1;
        case 0x94: return print_ZX("sty",addr + 1) + 1;
        case 0x95: return print_ZX("sta",addr + 1) + 1;
        case 0x96: return print_ZY("stx",addr + 1) + 1;
        case 0x98: return printf("tya\n") + 1;
        case 0x99: return print_Y("sta",addr + 1) + 1;
        case 0x9A: return printf("txs\n") + 1;
        case 0x9D: return print_X("sta",addr + 1) + 1;
        case 0xA0: return print_L("ldy",addr + 1) + 1;
        case 0xA1: return print_XM("lda",addr + 1) + 1;
        case 0xA2: return print_L("ldx",addr + 1) + 1;
        case 0xA4: return print_Z("ldy",addr + 1) + 1;
        case 0xA5: return print_Z("lda",addr + 1) + 1;
        case 0xA6: return print_Z("ldx",addr + 1) + 1;
        case 0xA8: return printf("tay\n") + 1;
        case 0xA9: return print_L("lda",addr + 1) + 1;
        case 0xAA: return printf("tax\n") + 1;
        case 0xAC: return print_A("ldy",addr + 1) + 1;
        case 0xAD: return print_A("lda",addr + 1) + 1;
        case 0xAE: return print_A("ldx",addr + 1) + 1;
        case 0xB0: return printf("bcs\n") + 1;
        case 0xB1: return print_MY("lda",addr + 1) + 1;
        case 0xB4: return print_ZX("ldy",addr + 1) + 1;
        case 0xB5: return print_ZX("lda",addr + 1) + 1;
        case 0xB6: return print_ZY("ldx",addr + 1) + 1;
        case 0xB8: return printf("clv\n") + 1;
        case 0xB9: return print_Y("lda",addr + 1) + 1;
        case 0xBA: return printf("tsx\n") + 1;
        case 0xBC: return print_X("ldy",addr + 1) + 1;
        case 0xBD: return print_X("lda",addr + 1) + 1;
        case 0xBE: return print_Y("ldx",addr + 1) + 1;
        case 0xC0: return print_L("cpy",addr + 1) + 1;
        case 0xC1: return print_XM("cmp",addr + 1) + 1;
        case 0xC4: return print_Z("cpy",addr + 1) + 1;
        case 0xC5: return print_Z("cmp",addr + 1) + 1;
        case 0xC6: return print_Z("dec",addr + 1) + 1;
        case 0xC8: return printf("iny\n") + 1;
        case 0xC9: return print_L("cmp",addr + 1) + 1;
        case 0xCA: return printf("dex\n") + 1;
        case 0xCC: return print_A("cpy",addr + 1) + 1;
        case 0xCD: return print_A("cmp",addr + 1) + 1;
        case 0xCE: return print_A("dec",addr + 1) + 1;
        case 0xD0: return printf("bne\n") + 1;
        case 0xD1: return print_MY("cmp",addr + 1) + 1;
        case 0xD5: return print_Z("cmp",addr + 1) + 1;
        case 0xD6: return print_Z("dec",addr + 1) + 1;
        case 0xD8: return printf("cld\n") + 1;
        case 0xD9: return print_Y("cmp",addr + 1) + 1;
        case 0xDD: return print_X("cmp",addr + 1) + 1;
        case 0xDE: return print_X("dec",addr + 1) + 1;
        case 0xE0: return print_L("cpx",addr + 1) + 1;
        case 0xE1: return print_XM("sbc",addr + 1) + 1;
        case 0xE4: return print_Z("cpx",addr + 1) + 1;
        case 0xE5: return print_Z("sbc",addr + 1) + 1;
        case 0xE6: return print_Z("inc",addr + 1) + 1;
        case 0xE8: return printf("inx\n") + 1;
        case 0xE9: return print_L("sbc",addr + 1) + 1;
        case 0xEA: return printf("nop\n") + 1;
        case 0xEC: return print_A("cpx",addr + 1) + 1;
        case 0xED: return print_A("sbc",addr + 1) + 1;
        case 0xEE: return print_A("inc",addr + 1) + 1;
        case 0xF0: return printf("beq\n") + 1;
        case 0xF1: return print_MY("sbc",addr + 1) + 1;
        case 0xF5: return print_Z("sbc",addr + 1) + 1;
        case 0xF6: return print_Z("inc",addr + 1) + 1;
        case 0xF8: return printf("sed\n") + 1;
        case 0xF9: return print_Y("sbc",addr + 1) + 1;
        case 0xFD: return print_X("sbc",addr + 1) + 1;
        case 0xFE: return print_X("inc",addr + 1) + 1;
    }
}


#define zero(v)   ((v.value&0xff) == 0)
#define nega(v)   (v.s1)
#define overflow(r) (r.s1 != r.s2)
#define carry(r)    (r.s2)


/*!
 *      指令后缀
 *      指令后有: _L    表示这是立即数寻址指令.
 *      指令后有: _A    表示这是绝对寻址指令.
 *      指令后有: _Z    表示这是零页寻址指令.
 *      指令后有: _V    表示这是累加器寻址指令.
 *      指令后有: _R    表示这是隐含寻址指令.
 *      指令后有: _X    表示这是绝对X变址寻址指令.
 *      指令后有: _Y    表示这是绝对Y变址寻址指令.
 *      指令后有: _ZX   表示这是零页X变址寻址指令.
 *      指令后有: _ZY   表示这是零页Y变址寻址指令.
 *      指令后有: _M    表示这是间接寻址指令.
 *      指令后有: _XM   表示这是先变X后间接寻址指令.
 *      指令后有: _MY   表示这是先间接寻址后变Y指令.
 *      指令后有: _O    表示这是相对寻址指令.
 *      
 !*/

#define lda(v)      ({A.value = v;N = nega(A);Z = zero(A);})
#define lda_XM()    ({lda(XADDR());addPCOperand8();})
#define lda_Z()     ({lda(ZADDR());addPCOperand8();})
#define lda_L()     ({lda(LUI());addPCOperand8();})
#define lda_A()     ({lda(ADDR());addPCOperand16();})
#define lda_MY()    ({lda(YADDR());addPCOperand8();})
#define lda_ZX()    ({lda(ZADDRX());addPCOperand8();})
#define lda_X()     ({lda(ADDRX());addPCOperand16();})
#define lda_Y()     ({lda(ADDRY());addPCOperand16();})

#define ldx(v)      ({X.value = v;N = nega(X);Z = zero(X);})
#define ldx_L()     ({ldx(LUI());addPCOperand8();})
#define ldx_Z()     ({ldx(ZADDR());addPCOperand8();})
#define ldx_A()     ({ldx(ADDR());addPCOperand16();})
#define ldx_ZY()    ({ldx(ZADDRY());addPCOperand8();})
#define ldx_Y()     ({ldx(ADDRY());addPCOperand16();})

#define ldy(v)      ({Y.value = v;N = nega(Y);Z = zero(Y);})
#define ldy_L()     ({ldy(LUI());addPCOperand8();})
#define ldy_Z()     ({ldy(ZADDR());addPCOperand8();})
#define ldy_A()     ({ldy(ADDR());addPCOperand16();})
#define ldy_ZX()    ({ldy(ZADDRX());addPCOperand8();})
#define ldy_X()     ({ldy(ADDRX());addPCOperand16();})

#define sta(v)      ({v = A.value;})
#define sta_XM()    ({sta(XADDR());addPCOperand8();})
#define sta_Z()     ({sta(ZADDR());addPCOperand8();})
#define sta_A()     ({sta(ADDR());addPCOperand16();})
#define sta_MY()    ({sta(YADDR());addPCOperand8();})
#define sta_ZX()    ({sta(ZADDRX());addPCOperand8();})
#define sta_X()     ({sta(ADDRX());addPCOperand16();})
#define sta_Y()     ({sta(ADDRY());addPCOperand16();})

#define stx(v)      ({v = X.value;})
#define stx_Z()     ({stx(ZADDR());addPCOperand8();})
#define stx_A()     ({stx(ADDR());addPCOperand16();})
#define stx_ZY()    ({stx(ZADDRY());addPCOperand8();})

#define sty(v)      ({v = Y.value;})
#define sty_Z()     ({sty(ZADDR());addPCOperand8();})
#define sty_A()     ({sty(ADDR());addPCOperand16();})
#define sty_ZX()    ({sty(ZADDRX());addPCOperand8();})

#define tr(d,s)   ({d.value = s.value;})
#define tax()   ({tr(X,A);addPC();})
#define txa()   ({tr(A,X);addPC();})
#define tay()   ({tr(Y,A);addPC();})
#define tya()   ({tr(A,Y);addPC();})
#define tsx()   ({tr(X,S);addPC();})
#define txs()   ({tr(S,X);addPC();})

#define adc(v)    ({\
        Register r = {.value = v};\
        A.s2 = A.s1;r.s2 = r.s1;\
        A.slue += r.slue + C;\
        Z = zero(A);\
        N = A.s1;\
        V = overflow(A);\
        C = carry(A); })
#define adc_XM()    ({adc(XADDR());addPCOperand8();})
#define adc_Z()     ({adc(ZADDR());addPCOperand8();})
#define adc_L()     ({adc(LUI());addPCOperand8();})
#define adc_A()     ({adc(ADDR());addPCOperand16();})
#define adc_MY()    ({adc(YADDR());addPCOperand8();})
#define adc_ZX()    ({adc(ZADDRX());addPCOperand8();})
#define adc_Y()     ({adc(ADDRY());addPCOperand16();})
#define adc_X()     ({adc(ADDRX());addPCOperand16();})

#define sbc(v)    ({\
        Register r = {.value = v};\
        A.s2 = A.s1;r.s2 = r.s1;\
        A.slue -= (r.slue + C);\
        Z = zero(A);\
        N = A.s1;\
        V = overflow(A);\
        C = carry(A); })
#define sbc_XM()    ({sbc(XADDR());addPCOperand8();})
#define sbc_Z()     ({sbc(ZADDR());addPCOperand8();})
#define sbc_L()     ({sbc(LUI());addPCOperand8();})
#define sbc_A()     ({sbc(ADDR());addPCOperand16();})
#define sbc_MY()    ({sbc(YADDR());addPCOperand8();})
#define sbc_ZX()    ({sbc(ZADDRX());addPCOperand8();})
#define sbc_Y()     ({sbc(ADDRY());addPCOperand16();})
#define sbc_X()     ({sbc(ADDRX());addPCOperand16();})

#define inc(v)    ({\
        Register r = {.value = v};\
        r.slue++;\
        v = r.value;\
        Z = zero(r);\
        N = r.s1; })
#define inc_Z() ({inc(ZADDR());addPCOperand8();})
#define inc_A() ({inc(ADDR());addPCOperand16();})
#define inc_ZX() ({inc(ZADDRX());addPCOperand8();})
#define inc_X() ({inc(ADDRX());addPCOperand16();})

#define dec(v)    ({\
        Register r = {.value = v};\
        r.slue--;\
        v = r.value;\
        Z = zero(r);\
        N = r.s1; })
#define dec_Z() ({dec(ZADDR());addPCOperand8();})
#define dec_A() ({dec(ADDR());addPCOperand16();})
#define dec_ZX() ({dec(ZADDRX());addPCOperand8();})
#define dec_X() ({dec(ADDRX());addPCOperand16();})

#define inx()    ({\
        X.slue++;\
        Z = zero(X);\
        N = X.s1;\
        addPC();})

#define dex(r)    ({\
        X.slue--;\
        Z = zero(X);\
        N = X.s1;\
        addPC();})

#define iny()    ({\
        Y.slue++;\
        Z = zero(Y);\
        N = Y.s1;\
        addPC(); })

#define dey(r)    ({\
        Y.slue--;\
        Z = zero(Y);\
        N = Y.s1;\
        addPC();})

#define and(v) ({A.value &= v;N = nega(A);Z = zero(A);})
#define and_XM()    ({and(XADDR());addPCOperand8();})
#define and_Z()     ({and(ZADDR());addPCOperand8();})
#define and_L()     ({and(LUI());addPCOperand8();})
#define and_A()     ({and(ADDR());addPCOperand16();})
#define and_MY()    ({and(YADDR());addPCOperand8();})
#define and_ZX()    ({and(ZADDRX());addPCOperand8();})
#define and_X()     ({and(ADDRX());addPCOperand16();})
#define and_Y()     ({and(ADDRY());addPCOperand16();})

#define ora(v)      ({A.value |= v;N = nega(A);Z = zero(A);})
#define ora_XM()    ({ora(XADDR());addPCOperand8();})
#define ora_Z()     ({ora(ZADDR());addPCOperand8();})
#define ora_L()     ({ora(LUI());addPCOperand8();})
#define ora_A()     ({ora(ADDR());addPCOperand16();})
#define ora_MY()    ({ora(YADDR());addPCOperand8();})
#define ora_ZX()    ({ora(ZADDRX());addPCOperand8();})
#define ora_X()     ({ora(ADDRX());addPCOperand16();})
#define ora_Y()     ({ora(ADDRY());addPCOperand16();})

#define eor(v) ({A.value ^= v;N = nega(A);Z = zero(A);})
#define eor_XM()    ({eor(XADDR());addPCOperand8();})
#define eor_Z()     ({eor(ZADDR());addPCOperand8();})
#define eor_L()     ({eor(LUI());addPCOperand8();})
#define eor_A()     ({eor(ADDR());addPCOperand16();})
#define eor_MY()    ({eor(YADDR());addPCOperand8();})
#define eor_ZX()    ({eor(ZADDRX());addPCOperand8();})
#define eor_X()     ({eor(ADDRX());addPCOperand16();})
#define eor_Y()     ({eor(ADDRY());addPCOperand16();})

#define sec()   ({C = 1;addPC();})
#define clc()   ({C = 0;addPC();})
#define sed()   ({D = 1;addPC();})
#define cld()   ({D = 0;addPC();})
#define sei()   ({I = 1;addPC();})
#define cli()   ({I = 0;addPC();})
#define clv()   ({V = 0;addPC();})

#define cmp(v)    ({\
        Register r = {.value = v};\
        A.s2 = A.s1;r.s2 = r.s1;\
        r.slue = A.slue - (r.slue + D);\
        Z = zero(r);\
        N = r.s1;\
        C = carry(r); })
#define cmp_XM()    ({cmp(XADDR());addPCOperand8();})
#define cmp_Z()     ({cmp(ZADDR());addPCOperand8();})
#define cmp_L()     ({cmp(LUI());addPCOperand8();})
#define cmp_A()     ({cmp(ADDR());addPCOperand16();})
#define cmp_MY()    ({cmp(YADDR());addPCOperand8();})
#define cmp_ZX()    ({cmp(ZADDRX());addPCOperand8();})
#define cmp_X()     ({cmp(ADDRX());addPCOperand16();})
#define cmp_Y()     ({cmp(ADDRY());addPCOperand16();})

#define cpx(v)    ({\
        Register r = {.value = v};\
        X.s2 = X.s1;r.s2 = r.s1;\
        r.slue = X.slue - (r.slue + D);\
        Z = zero(r);\
        N = r.s1;\
        C = carry(r); })
#define cpx_Z()     ({cpx(ZADDR());addPCOperand8();})
#define cpx_L()     ({cpx(LUI());addPCOperand8();})
#define cpx_A()     ({cpx(ADDR());addPCOperand16();})

#define cpy(v)    ({\
        Register r = {.value = v};\
        Y.s2 = Y.s1;r.s2 = r.s1;\
        r.slue = Y.slue - (r.slue + D);\
        Z = zero(r);\
        N = r.s1;\
        C = carry(r);})
#define cpy_Z()     ({cpy(ZADDR());addPCOperand8();})
#define cpy_L()     ({cpy(LUI());addPCOperand8();})
#define cpy_A()     ({cpy(ADDR());addPCOperand16();})

#define bit(v)    ({\
        Register r = {.value = v};\
        N = r.s1;\
        V = (r.value >> 6) & 1;\
        r.slue = (A.slue &= r.slue);\
        Z = zero(r);})
#define bit_Z()     ({bit(ZADDR());addPCOperand8();})
#define bit_A()     ({bit(ADDR());addPCOperand16();})

#define asl(v)    ({\
        Register r = {.value = v};\
        C = r.s1;\
        r.value <<= 1;\
        N = r.s1;\
        Z = zero(r);\
        v = r.value;})
#define asl_R()    ({\
        C = A.s1;\
        A.value <<= 1;\
        N = A.s1;\
        Z = zero(A);\
        addPC();})
#define asl_Z()     ({asl(ZADDR());addPCOperand8();})
#define asl_A()     ({asl(ADDR());addPCOperand16();})
#define asl_ZX()    ({asl(ZADDRX());addPCOperand8();})
#define asl_X()     ({asl(ADDRX());addPCOperand16();})

#define lsr(v)    ({\
        Register r = {.value = v};\
        C = r.value & 1;\
        r.value >>= 1;\
        N = r.s1;\
        Z = zero(r);\
        v = r.value;})
#define lsr_R()    ({\
        C = A.value & 1;\
        A.value >>= 1;\
        N = A.s1;\
        Z = zero(A);\
        addPC();})
#define lsr_Z()     ({lsr(ZADDR());addPCOperand8();})
#define lsr_A()     ({lsr(ADDR());addPCOperand16();})
#define lsr_ZX()    ({lsr(ZADDRX());addPCOperand8();})
#define lsr_X()     ({lsr(ADDRX());addPCOperand16();})

#define rol(v)    ({\
        Register r = {.value = v};\
        r.slue <<= 1;\
        r.value |= C;\
        C = r.s2;\
        N = r.s1;\
        Z = zero(r);\
        v = r.value;})
#define rol_R()    ({\
        A.slue <<= 1;\
        A.value |= C;\
        C = A.s2;\
        N = A.s1;\
        Z = zero(A);\
        addPC();})
#define rol_Z()     ({rol(ZADDR());addPCOperand8();})
#define rol_A()     ({rol(ADDR());addPCOperand16();})
#define rol_ZX()    ({rol(ZADDRX());addPCOperand8();})
#define rol_X()     ({rol(ADDRX());addPCOperand16();})

#define ror(v)    ({\
        Register r = {.value = v};\
        r.s2 = C;\
        C = r.value & 1;\
        r.slue >>= 1;\
        N = r.s1;\
        Z = zero(r);\
        v = r.value;})
#define ror_R()    ({\
        A.s2 = C;\
        C = A.value & 1;\
        A.slue >>= 1;\
        N = A.s1;\
        Z = zero(A);\
        addPC();})
#define ror_Z()     ({ror(ZADDR());addPCOperand8();})
#define ror_A()     ({ror(ADDR());addPCOperand16();})
#define ror_ZX()    ({ror(ZADDRX());addPCOperand8();})
#define ror_X()     ({ror(ADDRX());addPCOperand16();})

#define push(r) ({\
        S.value--;\
        STACK8 = r.value;})

#define pop(r)  ({\
        r.value = STACK8;\
        N = r.s1;\
        Z = zero(r);\
        S.value++;})

#define pha()   ({push(A);addPC();})
#define pla()   ({pop(A);addPC();})
#define php()   ({push(P);addPC();})
#define plp()   ({pop(P);addPC();})

#define jmp_A()    ({PC = OPERAND16;})
#define jmp_M()    ({PC = RAM[OPERAND8];})

#define cjmp(v) ({if(v){PC += OFFSET();}addPCOperand8();})
#define beq()   cjmp((Z))
#define bne()   cjmp((!Z))
#define bcs()   cjmp((C))
#define bcc()   cjmp((!C))
#define bmi()   cjmp((N))
#define bpl()   cjmp((!N))
#define bvs()   cjmp((V))
#define bvc()   cjmp((!V))

#define jsr()   ({\
        S.value -= 2;\
        STACK16 = PC + 3;\
        PC = OPERAND16;})
#define rts()   ({\
        PC = STACK16;\
        S.value += 2;})

/*! TODO: !*/

#define _int(v) ({\
        S.value--;\
        STACK8 = P.value;\
        S.value -= 2;\
        STACK16 = PC + 1;\
        PC = VALUE16(v);})

#define nmi() _int(0xFFFA)
#define rst() _int(0xFFFC)
#define brk() ({B = 1;_int(0xFFFE);})
#define irq() ({\
        B = 0;\
        S.value--;\
        STACK8 = P.value;\
        S.value -= 2;\
        STACK16 = PC;\
        PC = VALUE16(0xFFFE);})
#define rti()   ({\
        PC = STACK16;\
        S.value += 2;\
        P.value = STACK8;\
        S.value++;})
#define nop()   ({addPC();})


#define INPUT_IO    0x4016
void input(char ch){
    RAM[INPUT_IO] = ch;
    irq();
}

void output(char ch){
    putchar(ch);
}

void runCpu(void (*fn)(void),void (*err)(void)){
    if(fn) fn();
    disasm(PC);
    switch(RAM[PC]){
        default: 
            {
                size_t len = ((0x10000 - PC) / 256) ? 256 : (0x10000 - PC) % 256;
                printf("\e[31mInvalid operate %02X in %04X\n\e[00m",RAM[PC],PC);
                print_hex(RAM + PC,len,PC);
                print_hex(RAM + 0xFF00,256,0xFF00);
                print_hex(STACK + S.value - 0x10,0xFF - S.value + 0x10,0x100 + S.value - 0x10);
                if(err) err();
                break;
            }
        case 0x00: brk(); break;
        case 0x01: ora_XM(); break;
        case 0x05: ora_Z(); break;
        case 0x06: asl_Z(); break;
        case 0x08: php(); break;
        case 0x09: ora_L(); break;
        case 0x0A: asl_R(); break;
        case 0x0D: ora_A(); break;
        case 0x0E: asl_A(); break;
        case 0x10: bpl(); break;
        case 0x11: ora_MY(); break;
        case 0x15: ora_ZX(); break;
        case 0x16: asl_ZX(); break;
        case 0x18: clc(); break;
        case 0x19: ora_Y(); break;
        case 0x1D: ora_X(); break;
        case 0x1E: asl_X(); break;
        case 0x20: jsr(); break;
        case 0x21: and_XM(); break;
        case 0x24: bit_Z(); break;
        case 0x25: and_Z();break;
        case 0x26: rol_Z();break;
        case 0x28: plp();break;
        case 0x29: and_L();break;
        case 0x2A: rol_R();break;
        case 0x2C: bit_A();break;
        case 0x2D: and_A();break; 
        case 0x2E: rol_A();break;
        case 0x30: bmi();break;
        case 0x31: and_MY();break;
        case 0x35: and_ZX();break;
        case 0x36: rol_ZX();break;
        case 0x38: sec();break;
        case 0x39: and_Y();break;
        case 0x3D: and_X();break;
        case 0x3E: rol_X();break;
        case 0x40: rti();break;
        case 0x41: eor_XM();break;
        case 0x45: eor_Z();break;
        case 0x46: lsr_Z();break;
        case 0x48: pha();break;
        case 0x49: eor_L();break;
        case 0x4A: lsr_R();break;
        case 0x4C: jmp_A();break;
        case 0x4D: eor_A();break;
        case 0x4E: lsr_A();break;
        case 0x50: bvc();break;
        case 0x51: eor_MY();break;
        case 0x55: eor_ZX();break;
        case 0x56: lsr_ZX();break;
        case 0x58: cli();break;
        case 0x59: eor_Y();break;
        case 0x5D: eor_X();break;
        case 0x5E: lsr_X();break;
        case 0x60: rts();break;
        case 0x61: adc_XM();break;
        case 0x65: adc_Z();break;
        case 0x66: ror_Z();break;
        case 0x68: pla();break;
        case 0x69: adc_L();break;
        case 0x6A: ror_R();break;
        case 0x6C: jmp_M();break;
        case 0x6D: adc_A();break;
        case 0x6E: ror_A();break;
        case 0x70: bvs();break;
        case 0x71: adc_MY();break;
        case 0x75: adc_ZX();break;
        case 0x76: ror_ZX();break;
        case 0x78: sei();break;
        case 0x79: adc_Y();break;
        case 0x7D: adc_X();break;
        case 0x7E: ror_X();break;
        case 0x81: sta_XM();break;
        case 0x84: sty_Z();break;
        case 0x85: sta_Z();break;
        case 0x86: stx_Z();break;
        case 0x88: dey();break;
        case 0x8A: txa();break;
        case 0x8C: sty_A();break;
        case 0x8D: sta_A();break;
        case 0x8E: stx_A();break;
        case 0x90: bcc();break;
        case 0x91: sta_MY();break;
        case 0x94: sty_ZX();break;
        case 0x95: sta_ZX();break;
        case 0x96: stx_ZY();break;
        case 0x98: tya();break;
        case 0x99: sta_Y();break;
        case 0x9A: txs();break;
        case 0x9D: sta_X();break;
        case 0xA0: ldy_L();break;
        case 0xA1: lda_XM();break;
        case 0xA2: ldx_L();break;
        case 0xA4: ldy_Z();break;
        case 0xA5: lda_Z();break;
        case 0xA6: ldx_Z();break;
        case 0xA8: tay();break;
        case 0xA9: lda_L();break;
        case 0xAA: tax();break;
        case 0xAC: ldy_A();break;
        case 0xAD: lda_A();break;
        case 0xAE: ldx_A();break;
        case 0xB0: bcs();break;
        case 0xB1: lda_MY();break;
        case 0xB4: ldy_ZX();break;
        case 0xB5: lda_ZX();break;
        case 0xB6: ldx_ZY();break;
        case 0xB8: clv();break;
        case 0xB9: lda_Y();break;
        case 0xBA: tsx();break;
        case 0xBC: ldy_X();break;
        case 0xBD: lda_X();break;
        case 0xBE: ldx_Y();break;
        case 0xC0: cpy_L();break;
        case 0xC1: cmp_XM();break;
        case 0xC4: cpy_Z();break;
        case 0xC5: cmp_Z();break;
        case 0xC6: dec_Z();break;
        case 0xC8: iny();break;
        case 0xC9: cmp_L();break;
        case 0xCA: dex();break;
        case 0xCC: cpy_A();break;
        case 0xCD: cmp_A();break;
        case 0xCE: dec_A();break;
        case 0xD0: bne();break;
        case 0xD1: cmp_MY();break;
        case 0xD5: cmp_Z();break;
        case 0xD6: dec_Z();break;
        case 0xD8: cld();break;
        case 0xD9: cmp_Y();break;
        case 0xDD: cmp_X();break;
        case 0xDE: dec_X();break;
        case 0xE0: cpx_L();break;
        case 0xE1: sbc_XM();break;
        case 0xE4: cpx_Z();break;
        case 0xE5: sbc_Z();break;
        case 0xE6: inc_Z();break;
        case 0xE8: inx();break;
        case 0xE9: sbc_L();break;
        case 0xEA: nop();break;
        case 0xEC: cpx_A();break;
        case 0xED: sbc_A();break;
        case 0xEE: inc_A();break;
        case 0xF0: beq();break;
        case 0xF1: sbc_MY();break;
        case 0xF5: sbc_Z();break;
        case 0xF6: inc_Z();break;
        case 0xF8: sed();break;
        case 0xF9: sbc_Y();break;
        case 0xFD: sbc_X();break;
        case 0xFE: inc_X();break;
    }
}


void disassembly(uint16_t start,uint16_t size){
    for(uint16_t addr = start;addr < start + size;){
        addr += disasm(addr);
    }
}

void loadRom(const char *rom){
    FILE *fd = NULL;
    int index = 0;
    int cnt = 0;
    if(NULL == (fd = fopen(rom,"r"))){
        perror("Load program");
        exit(1);
    }
    powerOn();
    fseek(fd,0x10,SEEK_SET);
    while(!feof(fd) && (index < 0x8000)){
        if(-1 == (cnt =  fread(ram->softRom + index,1,1024,fd))){
            perror("Load program");
            fclose(fd);
            powerOff();
            exit(1);
        }
        index += cnt;
    }
    fclose(fd);
}
