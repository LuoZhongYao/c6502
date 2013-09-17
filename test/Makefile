.PHONY:debug all


all: test.nes
MAP = _test.map

CLEANFILES=test.nes

test.nes: test.c int.s
	$(CL) $(CLFLAGS)  -t nes -o  $@ $^ -g -d
	opencp $@ ~/Desktop
	opencp $@ ../

#test.bin: test.o 
#	$(LD) $(LDFLAGS) --lib c64.lib $^ -o $@ -C test.map

debug:
	$(CL) -S test.c -o test.s
include	/tools/Makefile.65
