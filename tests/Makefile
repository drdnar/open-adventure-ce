# Test-suite makefile for reposurgeon

# Use absolute path so tests that change working directory still use 
# scripts from parent directory.  Note that using $PWD seems to fail
# here under Gitlab's CI environment.
PATH := $(realpath ..):$(realpath .):${PATH}

# Defeat annoying behavior under Mac OS X - builtin echo doesn't do -n
ECHO := /bin/echo

all: regress
	@echo "=== No diff output is good news."

.SUFFIXES: .chk

clean:
	rm -fr *~ adventure.text

# Show summary lines for all tests.
testlist:
	@grep --text '^##' *.log
listcheck:
	@for f in *.log; do \
	    if ( head -3 $$f | grep --text -q '^ *##' ); then :; else echo "$$f needs a description"; fi; \
	done

# General regression testing of commands and output; look at the *.log and
# corresponding *.chk files to see which tests this runs.
TESTLOADS := $(shell ls -1 *.log | sed '/.log/s///')
buildregress:
	@for file in $(TESTLOADS); do \
	    echo "Remaking $${file}.chk"; \
	    advent <$${file}.log >$${file}.chk 2>&1 || exit 1; \
	done
regress:
	@for file in $(TESTLOADS); do \
	    $(ECHO) -n "  $${file} "; grep --text '##' $${file}.log  || echo ' ## (no description)'; \
	    if advent < $${file}.log >/tmp/regress$$$$ 2>&1; \
	    then diff --text -u $${file}.chk /tmp/regress$$$$ || exit 1; \
	    else echo "*** Nonzero return status on $${file}!"; exit 1; fi \
	done
	@rm -f /tmp/regress$$$$

# end
