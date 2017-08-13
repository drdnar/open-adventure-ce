# Test-suite makefile for reposurgeon

# Use absolute path so tests that change working directory still use
# scripts from parent directory.  Note that using $PWD seems to fail
# here under Gitlab's CI environment.
PARDIR=$(realpath ..)
PATH := $(PARDIR):$(realpath .):${PATH}
GCOV?=gcov

# Make this overrideable so it's easier to test old versions
advent?=advent

# Defeat annoying behavior under Mac OS X - builtin echo doesn't do -n
ECHO := /bin/echo

# Find all *.log entries to test
TESTLOADS := $(shell ls -1 *.log | sed '/.log/s///' | sort)

.PHONY: check coverage clean testlist listcheck savegames buildregress
.PHONY: savecheck regress

check: savecheck regress
	@echo "=== No diff output is good news."
	@-advent -x 2>/dev/null	# Get usage message into coverage tests
	@-advent -l /dev/null <pitfall.log >/dev/null

coverage: check
	lcov -t "advent" -o $(PARDIR)/advent.info -c -d $(PARDIR) --gcov-tool=$(GCOV)
	genhtml -o $(PARDIR)/coverage/ $(PARDIR)/advent.info
	./coverage_dungeon.py

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

# Generate bogus savegames.
savegames:
	@$(ECHO) "cheat: Generate save file with -900 deaths"
	@$(PARDIR)/cheat -d -900 -o cheat_numdie.adv > /tmp/cheat_numdie
	@$(ECHO) "cheat: Generate save file with -1000 deaths"
	@$(PARDIR)/cheat -d -1000 -o cheat_numdie1000.adv > /tmp/cheat_numdie1000
	@$(ECHO) "cheat: Generate save file with version -1337"
	@$(PARDIR)/cheat -v -1337 -o resume_badversion.adv > /tmp/cheat_badversion
	@$(ECHO) "cheat: Generate save file 1000 saves"
	@$(PARDIR)/cheat -s -1000 -o thousand_saves.adv > /tmp/cheat_1000saves
	@$(ECHO) "cheat: Generate save file 1000 turns"
	@$(PARDIR)/cheat -t -1000 -o thousand_saves.adv > /tmp/cheat_1000turns
	@$(ECHO) "cheat: Generate save file 1000 turns"
	@$(PARDIR)/cheat -l -1000 -o thousand_lamp.adv > /tmp/cheat_1000lamp
	@rm -f /tmp/cheat*


# Rebuild characterizing tests
buildregress: savegames
	$(PARDIR)/cheat -s -1000 -o thousand_saves.adv > /tmp/regress1000saves
	@for file in $(TESTLOADS); do \
	    echo "Remaking $${file}.chk"; \
	    OPTS=`sed -n /#options:/s///p <$${file}.log`; \
	    advent $$OPTS <$${file}.log >$${file}.chk 2>&1 || exit 1; \
	done; \
	rm -f scratch.tmp

savecheck: savegames
	@$(ECHO) "TEST cheat: Bogus option for save file generation"
	@$(PARDIR)/cheat -QqQ 2> /tmp/coverage_cheat_batopt | true
	@$(ECHO) "TEST cheat: No save file specified"
	@$(PARDIR)/cheat 2>/dev/null | true
	@$(ECHO) "TEST cheat: Fail to save because we omit -o"
	@$(PARDIR)/cheat -d 1 2> /tmp/coverage_cheat_nooutput | true
	@$(ECHO) "TEST cheat: Fail to save to invalid path"
	@$(PARDIR)/cheat -o / 2> /tmp/coverage_cheat_badoutput | true
	@$(ECHO) "TEST advent: Start with invalid file with -r"
	@advent -r /badfilename < pitfall.log > /tmp/coverage_advent_readfail 2>&1 || exit 1
	@$(ECHO) "TEST advent: Start with invalid file with -l"
	@advent -l / < pitfall.log > /tmp/coverage_advent_logfail 2>&1 || exit 1
	@$(ECHO) "TEST advent: Test -r with valid input"
	@advent -r thousand_saves.adv < pitfall.log > /tmp/coverage_advent_readfail 2>&1 || exit 1
	@rm -f /tmp/coverage*

# General regression testing of commands and output; look at the *.log and
# corresponding *.chk files to see which tests this runs.
regress:
	@for file in $(TESTLOADS); do \
	    $(ECHO) -n "  $${file} "; grep '##' $${file}.log  || echo ' ## (no description)'; \
	    OPTS=`sed -n /#options:/s///p <$${file}.log`; \
	    if $(advent) $$OPTS < $${file}.log >/tmp/regress$$$$ 2>&1; \
	    then diff --text -u $${file}.chk /tmp/regress$$$$ || exit 1; \
	    else echo "*** Nonzero return status on $${file}!"; exit 1; fi \
	done; \
	rm -f scratch.tmp /tmp/regress$$$$

# end
