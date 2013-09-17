
#CFLAGS += -g
nes.exe: main.o c6502.o clock.o
	gcc $^ -std=c99 -o $@

include /tools/Makefile
