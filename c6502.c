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

static void print_cpu(void){
    printf("C = %1d  Z = %1d  I = %1d  D = %1d  B = %1d  V = %1d  N = %1d\n",C,Z,I,D,B,V,N);
    printf("A = %02hhX X = %02hhX Y = %02hhX S = %02hhx  \e[31mPC = %04hX\e[0m\n",A.value,X.value,Y.value,S.value,PC);
}

void powerOn(void){
    ram = calloc(1,sizeof(CPURAM));
    if(ram == NULL){
        printf("Power on : Memory Out!\n");
        exit(1);
    }
}

void powerOff(void){
    if(ram) free(ram);
}


#define ADDR8(addr)     ((uint8_t*)(addr))
#define ADDR16(addr)    ((uint16_t *)(addr))

#define OPERAND8    (*ADDR8((RAM + PC + 1)))
#define OPERAND16   (*ADDR16((RAM + PC + 1)))

#define addPC()   (PC += 1)
#define addPCOperand8() (PC += 2)
#define addPCOperand16()    (PC += 3)


#define LUI()       (OPERAND8)                  /*! 立即数寻址 !*/
#define ADDR()      (RAM[OPERAND16])            /*! 直接寻址 !*/
#define ZADDR()     (RAM[OPERAND8])             /*! 零页寻址 !*/
#define AADDR(r)    (r.value)                   /*! 隐含寻址 !*/
#define ADDRX()     (RAM[OPERAND16 + X.value])  /*! X变址寻址 !*/
#define ADDRY()     (RAM[OPERAND16 + Y.value])  /*! Y变址寻址 !*/
#define ZADDRX()    (RAM[OPERAND8 + X.value])   /*! 零页X变址 !*/
#define ZADDRY()    (RAM[OPERAND8 + Y.value])   /*! 零页Y变址 !*/
#define XADDR()     (RAM[RAM[OPERAND8 + X.value] | (RAM[OPERAND8 + X.value + 1] << 8)])    /*! 先X变址，在间接寻址 !*/
#define YADDR()     (RAM[((RAM[OPERAND8]) | (RAM[OPERAND8 + 1] << 8)) + Y.value]) /*! 先间接寻址，在Y变址 !*/
#define OFFSET()    ((int8_t)(OPERAND8))  /*! 相对寻址 !*/

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
        Z = zero(A);})
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
        Z = zero(A);})
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
        Z = zero(A);})
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
        Z = zero(A);})
#define ror_Z()     ({ror(ZADDR());addPCOperand8();})
#define ror_A()     ({ror(ADDR());addPCOperand16();})
#define ror_ZX()    ({ror(ZADDRX());addPCOperand8();})
#define ror_X()     ({ror(ADDRX());addPCOperand16();})

#define push(r) ({\
        RAM[S.value] = r.value;\
        S.value--; })

#define pop(r)  ({\
        r.value = RAM[S.value];\
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

#define jsr()   ({S.value -= 2;RAM[S.value] = (PC + 3) & 0xFF;RAM[S.value + 1] = ((PC + 3)>> 8) & 0xff;PC = OPERAND16;})
#define rts()   ({PC = RAM[S.value]; PC |= (RAM[S.value+1] << 8);S.value += 2;})

/*! TODO: !*/
#define brk()   ({addPC();})
#define rti()   ({addPC();})
#define nop()   ({addPC();})



void run(void){
    while(1){
#if 1
        usleep(100000);
        print_cpu();
#endif
        switch(RAM[PC]){
            default: 
                {
                    size_t len = ((0x10000 - PC) / 256) ? 256 : (0x10000 - PC) % 256;
                    printf("\e[31mInvalid operate %02X in %04X\n\e[00m",RAM[PC],PC);
                    print_hex(RAM + PC,len,PC);
                    return;
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
}



int main(int argc,char **argv){
    FILE *fd = NULL;
    int index = 0;
    int cnt = 0;
    if(argc != 2){
        printf("usage : %s file\n",argv[0]);
        return 1;
    }
    printf("Loading progrom ...\n");
    if(NULL == (fd = fopen(argv[1],"r"))){
        perror("Load program");
        return 1;
    }
    powerOn();
    fseek(fd,0x10,SEEK_SET);
    while(!feof(fd) && (index < 0x8000)){
        if(-1 == (cnt =  fread(ram->softRom + index,1,1024,fd))){
            perror("Load program");
            fclose(fd);
            powerOff();
            return 1;
        }
        index += cnt;
    }
    PC = 0x8000;
    S.value = 0xFF;
    print_hex(ram->softRom,256,0x8000);
    run();
    fclose(fd);
    powerOff();
    return 0;
}
