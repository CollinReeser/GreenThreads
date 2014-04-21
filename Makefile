all: scheduler

callFunc.o: callFunc.asm
	nasm -f elf callFunc.asm

scheduler: scheduler.c callFunc.o
	gcc -m32 scheduler.c callFunc.o -o scheduler

realclean:
	-rm *.o
	-rm scheduler
