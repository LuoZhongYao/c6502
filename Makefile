
#CFLAGS += -g
c6502.exe: c6502.o
	gcc $^ -std=c99 -o $@

include /tools/Makefile
