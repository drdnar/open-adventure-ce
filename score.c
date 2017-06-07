#include <stdlib.h>
#include "advent.h"
#include "database.h"

/*
 * scoring and wrap-up
 */

void score(long mode)
/* mode is <0 if scoring, >0 if quitting, =0 if died or won */
{
    long i, score = 0, mxscor = 0;

    /*  The present scoring algorithm is as follows:
     *     Objective:          Points:        Present total possible:
     *  Getting well into cave   25                    25
     *  Each treasure < chest    12                    60
     *  Treasure chest itself    14                    14
     *  Each treasure > chest    16                   224
     *  Surviving             (MAX-NUM)*10             30
     *  Not quitting              4                     4
     *  Reaching "game.closng"   25                    25
     *  "Closed": Quit/Killed    10
     *            Klutzed        25
     *            Wrong way      30
     *            Success        45                    45
     *  Came to Witt's End        1                     1
     *  Round out the total       2                     2
     *                                       TOTAL:   430
     *  Points can also be deducted for using hints or too many turns, or for
     *  saving intermediate positions. */

    /*  First tally up the treasures.  Must be in building and not broken.
     *  Give the poor guy 2 points just for finding each treasure. */

    for (i=MINTRS; i<=MAXTRS; i++) {
	if(PTEXT[i] != 0) {
	    K=12;
	    if(i == CHEST)K=14;
	    if(i > CHEST)K=16;
	    if(game.prop[i] >= 0)
		score=score+2;
	    if(game.place[i] == 3 && game.prop[i] == 0)
		score=score+K-2;
	    mxscor=mxscor+K;
	}
    }

    /*  Now look at how he finished and how far he got.  MAXDIE and
     *  game.numdie tell us how well he survived.  game.dflag will tell us
     *  if he ever got suitably deep into the cave.  game.closng still
     *  indicates whether he reached the endgame.  And if he got as far as
     *  "cave closed" (indicated by "game.closed"), then bonus is zero for
     *  mundane exits or 133, 134, 135 if he blew it (so to speak). */

    score=score+(MAXDIE-game.numdie)*10;
    mxscor=mxscor+MAXDIE*10;
    if(mode == 0)score=score+4;
    mxscor=mxscor+4;
    if(game.dflag != 0)score=score+25;
    mxscor=mxscor+25;
    if(game.closng)score=score+25;
    mxscor=mxscor+25;
    if(game.closed) {
	if(game.bonus == 0)
	    score=score+10;
	if(game.bonus == 135)
	    score=score+25;
	if(game.bonus == 134)
	    score=score+30;
	if(game.bonus == 133)
	    score=score+45;
    }
    mxscor=mxscor+45;

    /* Did he come to Witt's End as he should? */
    if(game.place[MAGZIN] == 108)
	score=score+1;
    mxscor=mxscor+1;

    /* Round it off. */
    score=score+2;
    mxscor=mxscor+2;

    /* Deduct for hints/turns/saves. Hints < 4 are special; see database desc. */
    for (i=1; i<=HNTMAX; i++) {
	if(game.hinted[i])
	    score=score-HINTS[i][2];
    }
    if(game.novice)
	score=score-5;
    if(game.clshnt)
	score=score-10;
    score=score-game.trnluz-game.saved;

    /* Return to score command if that's where we came from. */
    if(mode < 0) {
	SETPRM(1,score,mxscor);
	SETPRM(3,game.turns,game.turns);
	RSPEAK(259);
	return;
    }

    /* that should be good enough.  Let's tell him all about it. */
    if(score+game.trnluz+1 >= mxscor && game.trnluz != 0)
	RSPEAK(242);
    if(score+game.saved+1 >= mxscor && game.saved != 0)
	RSPEAK(143);
    SETPRM(1,score,mxscor);
    SETPRM(3,game.turns,game.turns);
    RSPEAK(262);
    for (i=1; i<=CLSSES; i++) {
	if(CVAL[i] >= score) {
	    SPEAK(CTEXT[i]);
	    i=CVAL[i]+1-score;
	    SETPRM(1,i,i);
	    RSPEAK(263);
	    exit(0);
	}
    }
    RSPEAK(265);
    RSPEAK(264);
    exit(0);

}
