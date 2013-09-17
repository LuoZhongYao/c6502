#include    <stdio.h>
#include    <unistd.h>
#include    "c6502.h"
#include    "clock.h"

static void clockInt(int sig){
    lock();
    runCpu(NULL,uninitClock);
    unlock();
}


int main(int argc,char **argv){
    char ch;
    if(argc != 2){
        printf("usage : %s file\n",argv[0]);
        return 1;
    }
    printf("Loading progrom ...\n");
    loadRom(argv[1]);
    initClock(clockInt);
    powerOff();
    while('\n' != getchar());
    while(1){
        ch = getchar();
        lock();
        input(ch);
        unlock();
    }
    return 0;
}
