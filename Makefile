# Makefile for the open-source release of adventure 2.5

VERS=1.0

CC?=gcc
CCFLAGS+=-std=c99
LIBS=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBS=-lrt
endif

OBJS=main.o init.o actions1.o actions2.o score.o misc.o
SOURCES=$(OBJS:.o=.c) compile.c advent.h funcs.h adventure.text Makefile control

.c.o:
	$(CC) $(CCFLAGS) $(DBX) -c $<

advent:	$(OBJS) database.o
	$(CC) $(CCFLAGS) $(DBX) -o advent $(OBJS) database.o $(LDFLAGS) $(LIBS)

main.o:		advent.h funcs.h database.h

init.o:		advent.h funcs.h database.h

actions1.o:	advent.h funcs.h database.h

actions2.o:	advent.h funcs.h

score.o:	advent.h database.h

misc.o:		advent.h database.h

database.o:	database.h

compile: compile.c
	$(CC) $(CCFLAGS) -o $@ $<

database.c database.h: compile adventure.text
	./compile
	$(CC) $(CCFLAGS) $(DBX) -c database.c

html: index.html advent.html history.html hints.html

clean:
	rm -f *.o advent *.html database.[ch] compile *.gcno *.gcda
	rm -f README advent.6 MANIFEST
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

# Can't use GNU tar's --transform, needs to build under Alpine Linux
advent-$(VERS).tar.gz: $(SOURCES) $(DOCS) advent.6
	@ls $(SOURCES) $(DOCS) advent.1 | sed s:^:advent-$(VERS)/: >MANIFEST
	@(cd ..; ln -s advent advent-$(VERS))
	(cd ..; tar -czvf advent/advent-$(VERS).tar.gz `cat advent/MANIFEST`)
	@(cd ..; rm advent-$(VERS))

dist: advent-$(VERS).tar.gz

debug: CCFLAGS += -O0 --coverage
debug: advent
