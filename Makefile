# Makefile for the open-source release of adventure 2.5

VERS=1.0

CC?=gcc
CCFLAGS+=-std=c99
LIBS=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBS=-lrt
endif

OBJS=main.o init.o actions1.o actions2.o score.o misc.o database.o
SOURCES=$(OBJS:.o=.c) compile.c advent.h funcs.h adventure.text Makefile control

.c.o:
	$(CC) $(CCFLAGS) $(DBX) -c $<

advent:	$(OBJS) database.o
	$(CC) $(CCFLAGS) $(DBX) -o advent $(OBJS) $(LDFLAGS) $(LIBS)

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

html: index.html advent.html history.html hints.html

clean:
	rm -f *.o advent *.html database.[ch] compile *.gcno *.gcda
	rm -f README advent.6
	cd tests; $(MAKE) --quiet clean

check: advent
	cd tests; $(MAKE) --quiet

.SUFFIXES: .adoc .html .6

# Requires asciidoc and xsltproc/docbook stylesheets.
.adoc.6:
	a2x --doctype manpage --format manpage $<
.adoc.html:
	asciidoc $<
.adoc:
	asciidoc $<

# README.adoc exists because that filename is magic on GitLab.
DOCS=COPYING NEWS README.adoc TODO \
	advent.adoc history.adoc index.adoc hints.adoc advent.6

advent-$(VERS).tar.gz: $(SOURCES) $(DOCS)
	tar --transform='s:^:advent-$(VERS)/:' --show-transformed-names -cvzf advent-$(VERS).tar.gz $(SOURCES) $(DOCS)

dist: advent-$(VERS).tar.gz

debug: CCFLAGS += -O0 --coverage
debug: advent
