all: scheduler channels

channels: channel.h channel.c channelRunner.c
	gcc -o channels channelRunner.c channel.c

callFunc.o: callFunc.asm
	nasm -f elf callFunc.asm

scheduler: scheduler.c callFunc.o channel.h channel.c
	gcc -m32 scheduler.c callFunc.o -o scheduler channel.c

realclean:
	-rm *.o
	-rm scheduler
