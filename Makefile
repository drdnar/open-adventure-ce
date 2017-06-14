# Makefile for the open-source release of adventure 2.5

# Note: If you're building from the repository rather than the source tarball,
# do this to get the linenoise library where you can use it:
#
# git submodule update --recursive --remote --init
#
# Therafter, you can update it like this:
#
# git submodule update --recursive --remote
#
# but this should seldom be necessary as that library is pretty stable.

VERS=1.0

CC?=gcc
CCFLAGS+=-std=c99 -D _DEFAULT_SOURCE -Wall -Wpedantic -g
LIBS=
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIBS=-lrt
endif

OBJS=main.o init.o actions.o score.o misc.o saveresume.o common.o newdb.o
SOURCES=$(OBJS:.o=.c) dungeon.c advent.h common.h adventure.text Makefile control linenoise/linenoise.[ch] newdungeon.py

.c.o:
	$(CC) $(CCFLAGS) $(DBX) -c $<

advent:	$(OBJS) database.o linenoise.o
	$(CC) $(CCFLAGS) $(DBX) -o advent $(OBJS) database.o linenoise.o $(LDFLAGS) $(LIBS)

main.o:	 	advent.h database.h database.c common.h newdb.h

init.o:	 	advent.h database.h database.c common.h

actions.o:	advent.h database.h database.c common.h

score.o:	advent.h database.h database.c common.h

misc.o:		advent.h database.h database.c common.h newdb.h

saveresume.o:	advent.h database.h database.c common.h

common.o:	common.h

database.o:     database.c database.h common.h
	$(CC) $(CCFLAGS) $(DBX) -c database.c

newdb.o:	newdb.c newdb.h
	$(CC) $(CCFLAGS) $(DBX) -c newdb.c

database.c database.h: dungeon
	./dungeon

newdb.c newdb.h:
	./newdungeon.py

linenoise.o:	linenoise/linenoise.h
	$(CC) -c linenoise/linenoise.c

dungeon: dungeon.o common.o
	$(CC) $(CCFLAGS) -o $@ dungeon.o common.o

clean:
	rm -f *.o advent *.html database.[ch] dungeon *.gcno *.gcda
	rm -f newdb.c newdb.h
	rm -f README advent.6 MANIFEST *.tar.gz
	rm -f *~
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

html: advent.html history.html hints.html

# README.adoc exists because that filename is magic on GitLab.
DOCS=COPYING NEWS README.adoc TODO advent.adoc history.adoc notes.adoc hints.adoc advent.6
TESTFILES=tests/*.log tests/*.chk tests/README tests/decheck tests/Makefile

# Can't use GNU tar's --transform, needs to build under Alpine Linux.
# This is a requirement for testing dist in GitLab's CI pipeline
advent-$(VERS).tar.gz: $(SOURCES) $(DOCS)
	@find $(SOURCES) $(DOCS) $(TESTFILES) -print | sed s:^:advent-$(VERS)/: >MANIFEST
	@(ln -s . advent-$(VERS))
	(tar -T MANIFEST -czvf advent-$(VERS).tar.gz)
	@(rm advent-$(VERS))

release: advent-$(VERS).tar.gz advent.html history.html hints.html notes.html
	shipper version=$(VERS) | sh -e -x

refresh: advent.html
	shipper -N -w version=$(VERS) | sh -e -x

dist: advent-$(VERS).tar.gz

debug: CCFLAGS += -O0 --coverage -g
debug: advent
