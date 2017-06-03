# Makefile for the open-source release of adventure 2.5

CC?=gcc
CCFLAGS=-std=c99 -O0 --coverage
LIBS=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBS=-lrt
endif

OBJS=main.o init.o actions1.o actions2.o score.o misc.o database.o
DOCS=COPYING NEWS README TODO advent.adoc history.adoc index.adoc hints.adoc
SOURCES=$(OBJS:.o=.c) advent.h funcs.h adventure.text $(DOCS) control

.c.o:
	$(CC) $(CCFLAGS) $(DBX) -c $<

advent:	$(OBJS) database.o
	$(CC) $(CCFLAGS) $(DBX) -o advent $(OBJS) $(LIBS)

main.o:		advent.h funcs.h database.h

init.o:		advent.h funcs.h database.h

actions1.o:	advent.h funcs.h database.h

actions2.o:	advent.h funcs.h

score.o:	advent.h database.h

misc.o:		advent.h database.h

database.o:	database.h

funcs.h:	database.h

compile: compile.c
	$(CC) $(CCFLAGS) -o $@ $<

database.c database.h: compile adventure.text
	./compile
	$(CC) $(CCFLAGS) $(DBX) -c database.c

clean:
	rm -f *.o advent *.html advent.6 database.[ch] compile *.gcno *.gcda
	cd tests; $(MAKE) --quiet clean

check: advent
	cd tests; $(MAKE) --quiet

# Requires asciidoc and xsltproc/docbook stylesheets.
.asc.6: advent.adoc
	a2x --doctype manpage --format manpage $<
.asc.html: advent.adoc
	a2x --doctype manpage --format xhtml -D . $<
	rm -f docbook-xsl.css

advent-$(VERS).tar.gz: $(SOURCES) advent.6
	tar --transform='s:^:advent-$(VERS)/:' --show-transformed-names -cvzf advent-$(VERS).tar.gz $(SOURCES) advent.6

dist: advent-$(VERS).tar.gz

release: advent-$(VERS).tar.gz advent.html
	shipper version=$(VERS) | sh -e -x

refresh: advent.html
	shipper -N -w version=$(VERS) | sh -e -x
