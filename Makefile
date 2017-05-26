# Makefile for the open-source release of adventure 2.5

LIBS=-lrt
OBJS=main.o init.o actions1.o actions2.o score.o misc.o
SOURCES=$(OBJS:.o=.c) COPYING NEWS README TODO advent.text control

.c.o:
	gcc -O $(DBX) -c $<

advent:	$(OBJS)
	gcc -std=c99 -O $(DBX) -o advent $(OBJS) $(LIBS)

main.o:		misc.h funcs.h

init.o:		misc.h main.h share.h funcs.h

actions1.o:	misc.h main.h share.h funcs.h

actions2.o:	misc.h main.h share.h funcs.h

score.o:	misc.h main.h share.h

misc.o:		misc.h main.h

clean:
	rm -f *.o advent advent.html advent.6 adventure.data
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
