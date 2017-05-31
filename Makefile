# Makefile for the open-source release of adventure 2.5

CC?=gcc
CCFLAGS=-std=c99
LIBS=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBS=-lrt
endif

OBJS=main.o init.o actions1.o actions2.o score.o misc.o database.o
SOURCES=$(OBJS:.o=.c) COPYING NEWS README TODO adventure.text advent.text control misc.h main.h share.h funcs.h

.c.o:
	$(CC) $(CCFLAGS) -O $(DBX) -c $<

advent:	$(OBJS) database.o
	$(CC) $(CCFLAGS) -O $(DBX) -o advent $(OBJS) $(LIBS)

main.o:		main.h misc.h funcs.h database.h

init.o:		misc.h main.h share.h funcs.h database.h

actions1.o:	misc.h main.h share.h funcs.h database.h

actions2.o:	misc.h main.h share.h funcs.h

score.o:	misc.h main.h share.h database.h

misc.o:		misc.h main.h database.h

database.o:	database.h

funcs.h:	database.h

database.c database.h: compile adventure.text
	./compile
	$(CC) $(CCFLAGS) -O $(DBX) -c database.c

clean:
	rm -f *.o advent advent.html advent.6 database.[ch] compile
	cd tests; $(MAKE) --quiet clean

check: advent
	cd tests; $(MAKE) --quiet

# Requires asciidoc and xsltproc/docbook stylesheets.
.asc.6:
	a2x --doctype manpage --format manpage $<
.asc.html:
	a2x --doctype manpage --format xhtml -D . $<
	rm -f docbook-xsl.css

advent-$(VERS).tar.gz: $(SOURCES) advent.6
	tar --transform='s:^:advent-$(VERS)/:' --show-transformed-names -cvzf advent-$(VERS).tar.gz $(SOURCES) advent.6

dist: advent-$(VERS).tar.gz

release: advent-$(VERS).tar.gz advent.html
	shipper version=$(VERS) | sh -e -x

refresh: advent.html
	shipper -N -w version=$(VERS) | sh -e -x
