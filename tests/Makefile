# Test-suite makefile for reposurgeon

# Use absolute path so tests that change working directory still use 
# scripts from parent directory.  Note that using $PWD seems to fail
# here under Gitlab's CI environment.
PATH := $(realpath ..):$(realpath .):${PATH}

# Defeat annoying behavior under Mac OS X - builtin echo doesn't do -n
ECHO := /bin/echo

check: regress
	@echo "=== No diff output is good news."
	@-advent -x 2>/dev/null	# Get usage message into coverage tests
	@-advent -l /dev/null <quit.log >/dev/null

.SUFFIXES: .chk

clean:
	rm -fr *~ adventure.text *.adv scratch.tmp

# Show summary lines for all tests.
testlist:
	@grep '^##' *.log
listcheck:
	@for f in *.log; do \
	    if ( head -3 $$f | grep -q '^ *##' ); then :; else echo "$$f needs a description"; fi; \
	done

# General regression testing of commands and output; look at the *.log and
# corresponding *.chk files to see which tests this runs.
TESTLOADS := $(shell ls -1 *.log | sed '/.log/s///' | sort)
buildregress:
	@for file in $(TESTLOADS); do \
	    echo "Remaking $${file}.chk"; \
	    OPTS=`sed -n /#options:/s///p <$${file}.log`; \
	    advent $$OPTS <$${file}.log >$${file}.chk 2>&1 || exit 1; \
	done; \
	rm -f scratch.tmp
regress:
	$(ECHO) -n "Generate save file with -1000 deaths: "
	../cheat -d -1000 -o cheat_numdie.adv
	$(ECHO) -n "Generate save file with version -1337: "
	../cheat -v -1337 -o resume_badversion.adv
	$(ECHO) -n "Generate save file 1000 saves: "
	../cheat -s -1000 -o thousand_saves.adv
	$(ECHO) -n "Bogus option for save file generation: "
	../cheat -QqQ | true
	$(ECHO) -n "Fail to save because we omit -o: "
	../cheat -d 1 | true
	$(ECHO) -n "Fail to save to invalid path: "
	../cheat -o /badfilename.adv | true
	@for file in $(TESTLOADS); do \
	    $(ECHO) -n "  $${file} "; grep '##' $${file}.log  || echo ' ## (no description)'; \
	    OPTS=`sed -n /#options:/s///p <$${file}.log`; \
	    if advent $$OPTS < $${file}.log >/tmp/regress$$$$ 2>&1; \
	    then diff --text -u $${file}.chk /tmp/regress$$$$ || exit 1; \
	    else echo "*** Nonzero return status on $${file}!"; exit 1; fi \
	done; \
	rm -f scratch.tmp /tmp/regress$$$$

# end
