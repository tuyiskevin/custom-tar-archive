#CFLAGS=-Wall -Werror -g
CC=gcc
mytar: routines.o myTar.o inodemap.o
	$(CC) routines.o myTar.o inodemap.o -o  mytar

clean:
	$(RM) *.o