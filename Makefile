# Makefile for the pen-source release of adventure 2.4

OBJS=main.o init.o actions1.o actions2.o score.o misc.o datime.o

.c.o:
	gcc -O $(DBX) -c $<

adventure:	$(OBJS)
	gcc -O $(DBX) -o adventure $(OBJS)

main.o:		misc.h funcs.h

init.o:		misc.h main.h share.h funcs.h

actions1.o:	misc.h main.h share.h funcs.h

actions2.o:	misc.h main.h share.h funcs.h

score.o:	misc.h main.h share.h

misc.o:		misc.h main.h

clean:
	rm -f *.o adventure
