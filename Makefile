all: scheduler channels

channels: channel.h channel.c channelRunner.c
	gcc -o channels channelRunner.c channel.c

callFunc.o: callFunc.asm
	nasm -f elf64 callFunc.asm

scheduler: scheduler.c callFunc.o channel.h channel.c schedulerRunner.c scheduler.h
	gcc -o scheduler scheduler.c schedulerRunner.c channel.c callFunc.o

realclean:
	-rm *.o
	-rm scheduler
	-rm channels
