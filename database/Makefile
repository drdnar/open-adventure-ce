# Makefile for the open-adventure database

OBJS=compile.o
SOURCES=$(OBJS:.o=.c) adventure.text
OPTIONS=-ggdb -Wall -std=c99

.c.o:
	gcc $(OPTIONS) -O $(DBX) -c $<

database: compile
	./compile
	gcc $(OPTIONS) -O $(DBX) -c database.c

compile: $(OBJS)

clean:
	rm -f $(OBJS) compile database.c database.h
