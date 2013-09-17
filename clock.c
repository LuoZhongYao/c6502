#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>

static volatile int __lock = 0;
static void init_sigaction(void (*handler)(int)){
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    act.sa_flags |= SA_RESTART;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM,&act,NULL);
}

static void init_time(void) {
    struct itimerval value;
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 100;
    value.it_interval = value.it_value;
    setitimer(ITIMER_REAL,&value,NULL);
}

static void uninit_time(void){
    struct itimerval value;
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 0;
    value.it_interval = value.it_value;
    setitimer(ITIMER_REAL,&value,NULL);
}
extern void initClock(void(*handler)(int)){
    init_sigaction(handler);
    init_time();
}

extern void uninitClock(void){
    uninit_time();
    printf("Oop : I'm idle!\n");
    exit(1);
}

extern void lock(void){
    while(__lock);
    __lock = 1;
    uninit_time();
}

extern void unlock(void){
    init_time();
    __lock = 0;
}
