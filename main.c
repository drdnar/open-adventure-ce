/*
 * The author - Don Woods - apologises for the style of the code; it
 * is a result of running the original Fortran IV source through a
 * home-brew Fortran-to-C converter.)
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "main.h"

#include "misc.h"

long ABB[186], ATAB[331], ATLOC[186],
		DLOC[7], FIXED[101],
		KTAB[331], *LINES, LINK[201], LNLENG, LNPOSN,
		PARMS[26], PLACE[101], PTEXT[101], RTEXT[278],
		TABSIZ = 330;
signed char INLINE[LINESIZE+1], MAP1[129], MAP2[129];

long ACTVERB[36], AMBER, ATTACK, AXE, BACK, BATTER, BEAR, BIRD, BLOOD,
		 BOTTLE, CAGE, CAVE, CAVITY, CHAIN, CHASM, CHEST, CHLOC, CHLOC2,
		CLAM, CLSHNT, CLSMAX = 12, CLSSES,
		COINS, COND[186], CONDS, CTEXT[13], CVAL[13], DALTLC,
		DOOR, DPRSSN, DRAGON, DSEEN[7], DWARF, EGGS,
		EMRALD, ENTER, ENTRNC, FIND, FISSUR, FIXD[101], FOOD,
		GRATE, HINT, HINTED[21], HINTLC[21], HINTS[21][5], HNTMAX,
		HNTSIZ = 20, I, INVENT, IGO, J, JADE, K, K2, KEY[186], KEYS, KK,
		KNIFE, KQ, L, LAMP, LINSIZ = 12500, LINUSE, LL,
		LOC, LOCK, LOCSIZ = 185, LOCSND[186], LOOK, LTEXT[186],
		MAGZIN, MAXDIE, MAXTRS, MESH = 123456789,
		MESSAG, MIRROR, MXSCOR, NUGGET, NUL, OBJ, OBJSND[101],
		OBJTXT[101], ODLOC[7], OGRE, OIL, OYSTER,
	        PEARL, PILLOW, PLAC[101], PLANT, PLANT2, PROP[101], PYRAM,
		RESER, ROD, ROD2, RTXSIZ = 277, RUBY, RUG, SAPPH, SAY,
		SCORE, SECT, SIGN, SNAKE, STEPS, STEXT[186], STICK,
		STREAM, TABNDX, THROW, TK[21], TRAVEL[886], TRIDNT,
		TRNSIZ = 5, TRNVAL[6], TRNVLS, TROLL, TROLL2, TRVS,
		TRVSIZ = 885, TTEXT[6], URN, V1, V2, VASE, VEND, VERB,
		VOLCAN, VRBSIZ = 35, VRSION = 25, WATER, WD1, WD1X, WD2, WD2X;
struct game_t game = {.blklin = true};
FILE  *logfp;
bool oldstyle = false;

extern void initialise();
extern void score(long);
extern int action(FILE *, long);

/*
 * MAIN PROGRAM
 */

static void do_command(FILE *);

int main(int argc, char *argv[]) {
	int ch;

/*  Adventure (rev 2: 20 treasures) */

/*  History: Original idea & 5-treasure version (adventures) by Willie Crowther
 *           15-treasure version (adventure) by Don Woods, April-June 1977
 *           20-treasure version (rev 2) by Don Woods, August 1978
 *		Errata fixed: 78/12/25 */


/*  Options. */

	while ((ch = getopt(argc, argv, "l:o")) != EOF) {
		switch (ch) {
		case 'l':
			logfp = fopen(optarg, "w+");
			if (logfp == NULL)
				fprintf(stderr,
					"advent: can't open logfile %s for write\n",
					optarg);
			break;
		case 'o':
		    oldstyle = true;
		    break;
		}
	}

/* Logical variables:
 *
 *  game.closed says whether we're all the way closed
 *  game.closng says whether it's closing time yet
 *  CLSHNT says whether he's read the clue in the endgame
 *  game.lmwarn says whether he's been warned about lamp going dim
 *  game.novice says whether he asked for instructions at start-up
 *  game.panic says whether he's found out he's trapped in the cave
 *  game.wzdark says whether the loc he's leaving was dark */

#include "funcs.h"

/*  Read the database if we have not yet done so */

	LINES = (long *)calloc(LINSIZ+1,sizeof(long));
	if(!LINES){
		printf("Not enough memory!\n");
		exit(1);
	}

	MAP2[1] = 0;
	if(!game.setup)initialise();
	if(game.setup > 0) goto L1;

/*  Unlike earlier versions, adventure is no longer restartable.  (This
 *  lets us get away with modifying things such as OBJSND(BIRD) without
 *  having to be able to undo the changes later.)  If a "used" copy is
 *  rerun, we come here and tell the player to run a fresh copy. */

	RSPEAK(201);
	exit(0);

/*  Start-up, dwarf stuff */

L1:	game.setup= -1;
	I=RAN(-1);
	game.zzword=RNDVOC(3,0)+MESH*2;
	game.novice=YES(stdin, 65,1,0);
	game.newloc=1;
	LOC=1;
	game.limit=330;
	if(game.novice)game.limit=1000;

	for (;;) {
	    do_command(stdin);
	}
}

static void do_command(FILE *cmdin) {

/*  Can't leave cave once it's closing (except by main office). */

L2:	if(!OUTSID(game.newloc) || game.newloc == 0 || !game.closng) goto L71;
	RSPEAK(130);
	game.newloc=LOC;
	if(!game.panic)game.clock2=15;
	game.panic=true;

/*  See if a dwarf has seen him and has come from where he wants to go.  If so,
 *  the dwarf's blocking his way.  If coming from place forbidden to pirate
 *  (dwarves rooted in place) let him get out (and attacked). */

L71:	if(game.newloc == LOC || FORCED(LOC) || CNDBIT(LOC,3)) goto L74;
	/* 73 */ for (I=1; I<=5; I++) {
	if(ODLOC[I] != game.newloc || !DSEEN[I]) goto L73;
	game.newloc=LOC;
	RSPEAK(2);
	 goto L74;
L73:	/*etc*/ ;
	} /* end loop */
L74:	LOC=game.newloc;

/*  Dwarf stuff.  See earlier comments for description of variables.  Remember
 *  sixth dwarf is pirate and is thus very different except for motion rules. */

/*  First off, don't let the dwarves follow him into a pit or a wall.  Activate
 *  the whole mess the first time he gets as far as the hall of mists (loc 15).
 *  If game.newloc is forbidden to pirate (in particular, if it's beyond the troll
 *  bridge), bypass dwarf stuff.  That way pirate can't steal return toll, and
 *  dwarves can't meet the bear.  Also means dwarves won't follow him into dead
 *  end in maze, but c'est la vie.  They'll wait for him outside the dead end. */

	if(LOC == 0 || FORCED(LOC) || CNDBIT(game.newloc,3)) goto L2000;
	if(game.dflag != 0) goto L6000;
	if(INDEEP(LOC))game.dflag=1;
	 goto L2000;

/*  When we encounter the first dwarf, we kill 0, 1, or 2 of the 5 dwarves.  If
 *  any of the survivors is at loc, replace him with the alternate. */

L6000:	if(game.dflag != 1) goto L6010;
	if(!INDEEP(LOC) || (PCT(95) && (!CNDBIT(LOC,4) || PCT(85)))) goto L2000;
	game.dflag=2;
	for (I=1; I<=2; I++) {
	J=1+RAN(5);
	if(PCT(50))DLOC[J]=0;
	} /* end loop */
	for (I=1; I<=5; I++) {
	if(DLOC[I] == LOC)DLOC[I]=DALTLC;
	ODLOC[I]=DLOC[I];
	} /* end loop */
	RSPEAK(3);
	DROP(AXE,LOC);
	 goto L2000;

/*  Things are in full swing.  Move each dwarf at random, except if he's seen us
 *  he sticks with us.  Dwarves stay deep inside.  If wandering at random,
 *  they don't back up unless there's no alternative.  If they don't have to
 *  move, they attack.  And, of course, dead dwarves don't do much of anything. */

L6010:	game.dtotal=0;
	ATTACK=0;
	STICK=0;
	/* 6030 */ for (I=1; I<=6; I++) {
	if(DLOC[I] == 0) goto L6030;
/*  Fill TK array with all the places this dwarf might go. */
	J=1;
	KK=DLOC[I];
	KK=KEY[KK];
	if(KK == 0) goto L6016;
L6012:	game.newloc=MOD(IABS(TRAVEL[KK])/1000,1000);
	{long x = J-1;
	if(game.newloc > 300 || !INDEEP(game.newloc) || game.newloc == ODLOC[I] || (J > 1 &&
		game.newloc == TK[x]) || J >= 20 || game.newloc == DLOC[I] ||
		FORCED(game.newloc) || (I == 6 && CNDBIT(game.newloc,3)) ||
		IABS(TRAVEL[KK])/1000000 == 100) goto L6014;}
	TK[J]=game.newloc;
	J=J+1;
L6014:	KK=KK+1;
	{long x = KK-1; if(TRAVEL[x] >= 0) goto L6012;}
L6016:	TK[J]=ODLOC[I];
	if(J >= 2)J=J-1;
	J=1+RAN(J);
	ODLOC[I]=DLOC[I];
	DLOC[I]=TK[J];
	DSEEN[I]=(DSEEN[I] && INDEEP(LOC)) || (DLOC[I] == LOC || ODLOC[I] == LOC);
	if(!DSEEN[I]) goto L6030;
	DLOC[I]=LOC;
	if(I != 6) goto L6027;

/*  The pirate's spotted him.  He leaves him alone once we've found chest.  K
 *  counts if a treasure is here.  If not, and tally=1 for an unseen chest, let
 *  the pirate be spotted.  Note that PLACE(CHEST)=0 might mean that he's
 *  thrown it to the troll, but in that case he's seen the chest (PROP=0). */

	if(LOC == CHLOC || PROP[CHEST] >= 0) goto L6030;
	K=0;
	/* 6020 */ for (J=50; J<=MAXTRS; J++) {
/*  Pirate won't take pyramid from plover room or dark room (too easy!). */
	if(J == PYRAM && (LOC == PLAC[PYRAM] || LOC == PLAC[EMRALD])) goto L6020;
	if(TOTING(J)) goto L6021;
L6020:	if(HERE(J))K=1;
	} /* end loop */
	if(game.tally == 1 && K == 0 && PLACE[CHEST] == 0 && HERE(LAMP) && PROP[LAMP]
		== 1) goto L6025;
	if(ODLOC[6] != DLOC[6] && PCT(20))RSPEAK(127);
	 goto L6030;

L6021:	if(PLACE[CHEST] != 0) goto L6022;
/*  Install chest only once, to insure it is the last treasure in the list. */
	MOVE(CHEST,CHLOC);
	MOVE(MESSAG,CHLOC2);
L6022:	RSPEAK(128);
	/* 6023 */ for (J=50; J<=MAXTRS; J++) {
	if(J == PYRAM && (LOC == PLAC[PYRAM] || LOC == PLAC[EMRALD])) goto L6023;
	if(AT(J) && FIXED[J] == 0)CARRY(J,LOC);
	if(TOTING(J))DROP(J,CHLOC);
L6023:	/*etc*/ ;
	} /* end loop */
L6024:	DLOC[6]=CHLOC;
	ODLOC[6]=CHLOC;
	DSEEN[6]=false;
	 goto L6030;

L6025:	RSPEAK(186);
	MOVE(CHEST,CHLOC);
	MOVE(MESSAG,CHLOC2);
	 goto L6024;

/*  This threatening little dwarf is in the room with him! */

L6027:	game.dtotal=game.dtotal+1;
	if(ODLOC[I] != DLOC[I]) goto L6030;
	ATTACK=ATTACK+1;
	if(game.knfloc >= 0)game.knfloc=LOC;
	if(RAN(1000) < 95*(game.dflag-2))STICK=STICK+1;
L6030:	/*etc*/ ;
	} /* end loop */

/*  Now we know what's happening.  Let's tell the poor sucker about it.
 *  Note that various of the "knife" messages must have specific relative
 *  positions in the RSPEAK database. */

	if(game.dtotal == 0) goto L2000;
	SETPRM(1,game.dtotal,0);
	RSPEAK(4+1/game.dtotal);
	if(ATTACK == 0) goto L2000;
	if(game.dflag == 2)game.dflag=3;
	SETPRM(1,ATTACK,0);
	K=6;
	if(ATTACK > 1)K=250;
	RSPEAK(K);
	SETPRM(1,STICK,0);
	RSPEAK(K+1+2/(1+STICK));
	if(STICK == 0) goto L2000;
	game.oldlc2=LOC;
	 goto L99;






/*  Describe the current location and (maybe) get next command. */

/*  Print text for current loc. */

L2000:	if(LOC == 0) goto L99;
	KK=STEXT[LOC];
	if(MOD(ABB[LOC],game.abbnum) == 0 || KK == 0)KK=LTEXT[LOC];
	if(FORCED(LOC) || !DARK(0)) goto L2001;
	if(game.wzdark && PCT(35)) goto L90;
	KK=RTEXT[16];
L2001:	if(TOTING(BEAR))RSPEAK(141);
	SPEAK(KK);
	K=1;
	if(FORCED(LOC)) goto L8;
	if(LOC == 33 && PCT(25) && !game.closng)RSPEAK(7);

/*  Print out descriptions of objects at this location.  If not closing and
 *  property value is negative, tally off another treasure.  Rug is special
 *  case; once seen, its PROP is 1 (dragon on it) till dragon is killed.
 *  Similarly for chain; PROP is initially 1 (locked to bear).  These hacks
 *  are because PROP=0 is needed to get full score. */

	if(DARK(0)) goto L2012;
	ABB[LOC]=ABB[LOC]+1;
	I=ATLOC[LOC];
L2004:	if(I == 0) goto L2012;
	OBJ=I;
	if(OBJ > 100)OBJ=OBJ-100;
	if(OBJ == STEPS && TOTING(NUGGET)) goto L2008;
	if(PROP[OBJ] >= 0) goto L2006;
	if(game.closed) goto L2008;
	PROP[OBJ]=0;
	if(OBJ == RUG || OBJ == CHAIN)PROP[OBJ]=1;
	game.tally=game.tally-1;
/*  Note: There used to be a test here to see whether the player had blown it
 *  so badly that he could never ever see the remaining treasures, and if so
 *  the lamp was zapped to 35 turns.  But the tests were too simple-minded;
 *  things like killing the bird before the snake was gone (can never see
 *  jewelry), and doing it "right" was hopeless.  E.G., could cross troll
 *  bridge several times, using up all available treasures, breaking vase,
 *  using coins to buy batteries, etc., and eventually never be able to get
 *  across again.  If bottle were left on far side, could then never get eggs
 *  or trident, and the effects propagate.  So the whole thing was flushed.
 *  anyone who makes such a gross blunder isn't likely to find everything
 *  else anyway (so goes the rationalisation). */
L2006:	KK=PROP[OBJ];
	if(OBJ == STEPS && LOC == FIXED[STEPS])KK=1;
	PSPEAK(OBJ,KK);
L2008:	I=LINK[I];
	 goto L2004;

L2009:	K=54;
L2010:	game.spk=K;
L2011:	RSPEAK(game.spk);

L2012:	VERB=0;
	game.oldobj=OBJ;
	OBJ=0;

/*  Check if this loc is eligible for any hints.  If been here long enough,
 *  branch to help section (on later page).  Hints all come back here eventually
 *  to finish the loop.  Ignore "HINTS" < 4 (special stuff, see database notes).
		*/

L2600:	if(COND[LOC] < CONDS) goto L2603;
	/* 2602 */ for (HINT=1; HINT<=HNTMAX; HINT++) {
	if(HINTED[HINT]) goto L2602;
	if(!CNDBIT(LOC,HINT+10))HINTLC[HINT]= -1;
	HINTLC[HINT]=HINTLC[HINT]+1;
	if(HINTLC[HINT] >= HINTS[HINT][1]) goto L40000;
L2602:	/*etc*/ ;
	} /* end loop */

/*  Kick the random number generator just to add variety to the chase.  Also,
 *  if closing time, check for any objects being toted with PROP < 0 and set
 *  the prop to -1-PROP.  This way objects won't be described until they've
 *  been picked up and put down separate from their respective piles.  Don't
 *  tick game.clock1 unless well into cave (and not at Y2). */

L2603:	if(!game.closed) goto L2605;
	if(PROP[OYSTER] < 0 && TOTING(OYSTER))PSPEAK(OYSTER,1);
	for (I=1; I<=100; I++) {
	if(TOTING(I) && PROP[I] < 0)PROP[I]= -1-PROP[I];
	} /* end loop */
L2605:	game.wzdark=DARK(0);
	if(game.knfloc > 0 && game.knfloc != LOC)game.knfloc=0;
	I=RAN(1);
	GETIN(cmdin, WD1,WD1X,WD2,WD2X);

/*  Every input, check "game.foobar" flag.  If zero, nothing's going on.  If pos,
 *  make neg.  If neg, he skipped a word, so make it zero. */

L2607:	game.foobar=(game.foobar>0 ? -game.foobar : 0);
	game.turns=game.turns+1;
	if(game.turns != game.thresh) goto L2608;
	SPEAK(TTEXT[game.trndex]);
	game.trnluz=game.trnluz+TRNVAL[game.trndex]/100000;
	game.trndex=game.trndex+1;
	game.thresh= -1;
	if(game.trndex <= TRNVLS)game.thresh=MOD(TRNVAL[game.trndex],100000)+1;
L2608:	if(VERB == SAY && WD2 > 0)VERB=0;
	if(VERB == SAY) goto L4090;
	if(game.tally == 0 && INDEEP(LOC) && LOC != 33)game.clock1=game.clock1-1;
	if(game.clock1 == 0) goto L10000;
	if(game.clock1 < 0)game.clock2=game.clock2-1;
	if(game.clock2 == 0) goto L11000;
	if(PROP[LAMP] == 1)game.limit=game.limit-1;
	if(game.limit <= 30 && HERE(BATTER) && PROP[BATTER] == 0 && HERE(LAMP)) goto
		L12000;
	if(game.limit == 0) goto L12400;
	if(game.limit <= 30) goto L12200;
L19999: K=43;
	if(LIQLOC(LOC) == WATER)K=70;
	V1=VOCAB(WD1,-1);
	V2=VOCAB(WD2,-1);
	if(V1 == ENTER && (V2 == STREAM || V2 == 1000+WATER)) goto L2010;
	if(V1 == ENTER && WD2 > 0) goto L2800;
	if((V1 != 1000+WATER && V1 != 1000+OIL) || (V2 != 1000+PLANT && V2 !=
		1000+DOOR)) goto L2610;
	{long x = V2-1000; if(AT(x))WD2=MAKEWD(16152118);}
L2610:	if(V1 == 1000+CAGE && V2 == 1000+BIRD && HERE(CAGE) &&
		HERE(BIRD))WD1=MAKEWD(301200308);
L2620:	if(WD1 != MAKEWD(23051920)) goto L2625;
	game.iwest=game.iwest+1;
	if(game.iwest == 10)RSPEAK(17);
L2625:	if(WD1 != MAKEWD( 715) || WD2 == 0) goto L2630;
	IGO=IGO+1;
	if(IGO == 10)RSPEAK(276);
L2630:	I=VOCAB(WD1,-1);
	if(I == -1) goto L3000;
	K=MOD(I,1000);
	KQ=I/1000+1;
	 switch (KQ-1) { case 0: goto L8; case 1: goto L5000; case 2: goto L4000;
		case 3: goto L2010; }
	BUG(22);

/*  Get second word for analysis. */

L2800:	WD1=WD2;
	WD1X=WD2X;
	WD2=0;
	 goto L2620;

/*  Gee, I don't understand. */

L3000:	SETPRM(1,WD1,WD1X);
	RSPEAK(254);
	 goto L2600;

/* Verb and object analysis moved to separate module. */

L4000:	I=4000; goto Laction;
L4090:	I=4090; goto Laction;
L5000:	I=5000;
Laction:
	 switch (action(cmdin, I)) {
	   case 2: return;
	   case 8: goto L8;
	   case 2000: goto L2000;
	   case 2009: goto L2009;
	   case 2010: goto L2010;
	   case 2011: goto L2011;
	   case 2012: goto L2012;
	   case 2600: goto L2600;
	   case 2607: goto L2607;
	   case 2630: goto L2630;
	   case 2800: goto L2800;
	   case 8000: goto L8000;
	   case 18999: goto L18999;
	   case 19000: goto L19000;
	   }
	BUG(99);

/*  Random intransitive verbs come here.  Clear obj just in case (see "attack").
		*/

L8000:	SETPRM(1,WD1,WD1X);
	RSPEAK(257);
	OBJ=0;
	goto L2600;

/*  Figure out the new location
 *
 *  Given the current location in "LOC", and a motion verb number in
 *  "K", put the new location in "game.newloc".  The current loc is
 *  saved in "game.oldloc" in case he wants to retreat.  The current
 *  game.oldloc is saved in game.oldlc2, in case he dies.  (if he
 *  does, game.newloc will be limbo, and game.oldloc will be what
 *  killed him, so we need game.oldlc2, which is the last place he was
 *  safe.) */

L8:	KK=KEY[LOC];
	game.newloc=LOC;
	if(KK == 0)BUG(26);
	if(K == NUL) return;
	if(K == BACK) goto L20;
	if(K == LOOK) goto L30;
	if(K == CAVE) goto L40;
	game.oldlc2=game.oldloc;
	game.oldloc=LOC;

L9:	LL=IABS(TRAVEL[KK]);
	if(MOD(LL,1000) == 1 || MOD(LL,1000) == K) goto L10;
	if(TRAVEL[KK] < 0) goto L50;
	KK=KK+1;
	 goto L9;

L10:	LL=LL/1000;
L11:	game.newloc=LL/1000;
	K=MOD(game.newloc,100);
	if(game.newloc <= 300) goto L13;
	if(PROP[K] != game.newloc/100-3) goto L16;
L12:	if(TRAVEL[KK] < 0)BUG(25);
	KK=KK+1;
	game.newloc=IABS(TRAVEL[KK])/1000;
	if(game.newloc == LL) goto L12;
	LL=game.newloc;
	 goto L11;

L13:	if(game.newloc <= 100) goto L14;
	if(TOTING(K) || (game.newloc > 200 && AT(K))) goto L16;
	 goto L12;

L14:	if(game.newloc != 0 && !PCT(game.newloc)) goto L12;
L16:	game.newloc=MOD(LL,1000);
	if(game.newloc <= 300) return;
	if(game.newloc <= 500) goto L30000;
	RSPEAK(game.newloc-500);
	game.newloc=LOC;
	 return;

/*  Special motions come here.  Labelling convention: statement numbers NNNXX
 *  (XX=00-99) are used for special case number NNN (NNN=301-500). */

L30000: game.newloc=game.newloc-300;
	 switch (game.newloc) { case 1: goto L30100; case 2: goto L30200; case 3: goto
		L30300; }
	BUG(20);

/*  Travel 301.  Plover-alcove passage.  Can carry only emerald.  Note: travel
 *  table must include "useless" entries going through passage, which can never
 *  be used for actual motion, but can be spotted by "go back". */

L30100: game.newloc=99+100-LOC;
	if(game.holdng == 0 || (game.holdng == 1 && TOTING(EMRALD))) return;
	game.newloc=LOC;
	RSPEAK(117);
	 return;

/*  Travel 302.  Plover transport.  Drop the emerald (only use special travel if
 *  toting it), so he's forced to use the plover-passage to get it out.  Having
 *  dropped it, go back and pretend he wasn't carrying it after all. */

L30200: DROP(EMRALD,LOC);
	 goto L12;

/*  Travel 303.  Troll bridge.  Must be done only as special motion so that
 *  dwarves won't wander across and encounter the bear.  (They won't follow the
 *  player there because that region is forbidden to the pirate.)  If
 *  PROP(TROLL)=1, he's crossed since paying, so step out and block him.
 *  (standard travel entries check for PROP(TROLL)=0.)  Special stuff for bear. */

L30300: if(PROP[TROLL] != 1) goto L30310;
	PSPEAK(TROLL,1);
	PROP[TROLL]=0;
	MOVE(TROLL2,0);
	MOVE(TROLL2+100,0);
	MOVE(TROLL,PLAC[TROLL]);
	MOVE(TROLL+100,FIXD[TROLL]);
	JUGGLE(CHASM);
	game.newloc=LOC;
	 return;

L30310: game.newloc=PLAC[TROLL]+FIXD[TROLL]-LOC;
	if(PROP[TROLL] == 0)PROP[TROLL]=1;
	if(!TOTING(BEAR)) return;
	RSPEAK(162);
	PROP[CHASM]=1;
	PROP[TROLL]=2;
	DROP(BEAR,game.newloc);
	FIXED[BEAR]= -1;
	PROP[BEAR]=3;
	game.oldlc2=game.newloc;
	 goto L99;

/*  End of specials. */

/*  Handle "go back".  Look for verb which goes from LOC to
 *  game.oldloc, or to game.oldlc2 If game.oldloc has forced-motion.
 *  K2 saves entry -> forced loc -> previous loc. */

L20:	K=game.oldloc;
	if(FORCED(K))K=game.oldlc2;
	game.oldlc2=game.oldloc;
	game.oldloc=LOC;
	K2=0;
	if(K == LOC)K2=91;
	if(CNDBIT(LOC,4))K2=274;
	if(K2 == 0) goto L21;
	RSPEAK(K2);
	 return;

L21:	LL=MOD((IABS(TRAVEL[KK])/1000),1000);
	if(LL == K) goto L25;
	if(LL > 300) goto L22;
	J=KEY[LL];
	if(FORCED(LL) && MOD((IABS(TRAVEL[J])/1000),1000) == K)K2=KK;
L22:	if(TRAVEL[KK] < 0) goto L23;
	KK=KK+1;
	 goto L21;

L23:	KK=K2;
	if(KK != 0) goto L25;
	RSPEAK(140);
	 return;

L25:	K=MOD(IABS(TRAVEL[KK]),1000);
	KK=KEY[LOC];
	 goto L9;

/*  Look.  Can't give more detail.  Pretend it wasn't dark (though it may "now"
 *  be dark) so he won't fall into a pit while staring into the gloom. */

L30:	if(game.detail < 3)RSPEAK(15);
	game.detail=game.detail+1;
	game.wzdark=false;
	ABB[LOC]=0;
	 return;

/*  Cave.  Different messages depending on whether above ground. */

L40:	K=58;
	if(OUTSID(LOC) && LOC != 8)K=57;
	RSPEAK(K);
	 return;

/*  Non-applicable motion.  Various messages depending on word given. */

L50:	game.spk=12;
	if(K >= 43 && K <= 50)game.spk=52;
	if(K == 29 || K == 30)game.spk=52;
	if(K == 7 || K == 36 || K == 37)game.spk=10;
	if(K == 11 || K == 19)game.spk=11;
	if(VERB == FIND || VERB == INVENT)game.spk=59;
	if(K == 62 || K == 65)game.spk=42;
	if(K == 17)game.spk=80;
	RSPEAK(game.spk);
	 return;





/*  "You're dead, Jim."
 *
 *  If the current loc is zero, it means the clown got himself killed.  We'll
 *  allow this maxdie times.  MAXDIE is automatically set based on the number of
 *  snide messages available.  Each death results in a message (81, 83, etc.)
 *  which offers reincarnation; if accepted, this results in message 82, 84,
 *  etc.  The last time, if he wants another chance, he gets a snide remark as
 *  we exit.  When reincarnated, all objects being carried get dropped at game.oldlc2
 *  (presumably the last place prior to being killed) without change of props.
 *  the loop runs backwards to assure that the bird is dropped before the cage.
 *  (this kluge could be changed once we're sure all references to bird and cage
 *  are done by keywords.)  The lamp is a special case (it wouldn't do to leave
 *  it in the cave).  It is turned off and left outside the building (only if he
 *  was carrying it, of course).  He himself is left inside the building (and
 *  heaven help him if he tries to xyzzy back into the cave without the lamp!).
 *  game.oldloc is zapped so he can't just "retreat". */

/*  The easiest way to get killed is to fall into a pit in pitch darkness. */

L90:	RSPEAK(23);
	game.oldlc2=LOC;

/*  Okay, he's dead.  Let's get on with it. */

L99:	if(game.closng) goto L95;
	game.numdie=game.numdie+1;
	if(!YES(cmdin,79+game.numdie*2,80+game.numdie*2,54)) score(0);
	if(game.numdie == MAXDIE) score(0);
	PLACE[WATER]=0;
	PLACE[OIL]=0;
	if(TOTING(LAMP))PROP[LAMP]=0;
	/* 98 */ for (J=1; J<=100; J++) {
	I=101-J;
	if(!TOTING(I)) goto L98;
	K=game.oldlc2;
	if(I == LAMP)K=1;
	DROP(I,K);
L98:	/*etc*/ ;
	} /* end loop */
	LOC=3;
	game.oldloc=LOC;
	 goto L2000;

/*  He died during closing time.  No resurrection.  Tally up a death and exit. */

L95:	RSPEAK(131);
	game.numdie=game.numdie+1;
	 score(0);




/*  Hints */

/*  Come here if he's been long enough at required loc(s) for some unused hint.
 *  hint number is in variable "hint".  Branch to quick test for additional
 *  conditions, then come back to do neat stuff.  Goto 40010 if conditions are
 *  met and we want to offer the hint.  Goto 40020 to clear HINTLC back to zero,
 *  40030 to take no action yet. */

L40000:    switch (HINT-1) { case 0: goto L40100; case 1: goto L40200; case 2: goto
		L40300; case 3: goto L40400; case 4: goto L40500; case 5: goto
		L40600; case 6: goto L40700; case 7: goto L40800; case 8: goto
		L40900; case 9: goto L41000; }
/*		CAVE  BIRD  SNAKE MAZE  DARK  WITT  URN   WOODS OGRE
 *		JADE */
	BUG(27);

L40010: HINTLC[HINT]=0;
	if(!YES(cmdin,HINTS[HINT][3],0,54)) goto L2602;
	SETPRM(1,HINTS[HINT][2],HINTS[HINT][2]);
	RSPEAK(261);
	HINTED[HINT]=YES(cmdin,175,HINTS[HINT][4],54);
	if(HINTED[HINT] && game.limit > 30)game.limit=game.limit+30*HINTS[HINT][2];
L40020: HINTLC[HINT]=0;
L40030:  goto L2602;

/*  Now for the quick tests.  See database description for one-line notes. */

L40100: if(PROP[GRATE] == 0 && !HERE(KEYS)) goto L40010;
	 goto L40020;

L40200: if(PLACE[BIRD] == LOC && TOTING(ROD) && game.oldobj == BIRD) goto L40010;
	 goto L40030;

L40300: if(HERE(SNAKE) && !HERE(BIRD)) goto L40010;
	 goto L40020;

L40400: if(ATLOC[LOC] == 0 && ATLOC[game.oldloc] == 0 && ATLOC[game.oldlc2] == 0 && game.holdng >
		1) goto L40010;
	 goto L40020;

L40500: if(PROP[EMRALD] != -1 && PROP[PYRAM] == -1) goto L40010;
	 goto L40020;

L40600:  goto L40010;

L40700: if(game.dflag == 0) goto L40010;
	 goto L40020;

L40800: if(ATLOC[LOC] == 0 && ATLOC[game.oldloc] == 0 && ATLOC[game.oldlc2] == 0) goto
		L40010;
	 goto L40030;

L40900: I=ATDWRF(LOC);
	if(I < 0) goto L40020;
	if(HERE(OGRE) && I == 0) goto L40010;
	 goto L40030;

L41000: if(game.tally == 1 && PROP[JADE] < 0) goto L40010;
	 goto L40020;





/*  Cave closing and scoring */


/*  These sections handle the closing of the cave.  The cave closes "clock1"
 *  turns after the last treasure has been located (including the pirate's
 *  chest, which may of course never show up).  Note that the treasures need not
 *  have been taken yet, just located.  Hence clock1 must be large enough to get
 *  out of the cave (it only ticks while inside the cave).  When it hits zero,
 *  we branch to 10000 to start closing the cave, and then sit back and wait for
 *  him to try to get out.  If he doesn't within clock2 turns, we close the
 *  cave; if he does try, we assume he panics, and give him a few additional
 *  turns to get frantic before we close.  When clock2 hits zero, we branch to
 *  11000 to transport him into the final puzzle.  Note that the puzzle depends
 *  upon all sorts of random things.  For instance, there must be no water or
 *  oil, since there are beanstalks which we don't want to be able to water,
 *  since the code can't handle it.  Also, we can have no keys, since there is a
 *  grate (having moved the fixed object!) there separating him from all the
 *  treasures.  Most of these problems arise from the use of negative prop
 *  numbers to suppress the object descriptions until he's actually moved the
 *  objects. */

/*  When the first warning comes, we lock the grate, destroy the bridge, kill
 *  all the dwarves (and the pirate), remove the troll and bear (unless dead),
 *  and set "closng" to true.  Leave the dragon; too much trouble to move it.
 *  from now until clock2 runs out, he cannot unlock the grate, move to any
 *  location outside the cave, or create the bridge.  Nor can he be
 *  resurrected if he dies.  Note that the snake is already gone, since he got
 *  to the treasure accessible only via the hall of the mountain king. Also, he's
 *  been in giant room (to get eggs), so we can refer to it.  Also also, he's
 *  gotten the pearl, so we know the bivalve is an oyster.  *And*, the dwarves
 *  must have been activated, since we've found chest. */

L10000: PROP[GRATE]=0;
	PROP[FISSUR]=0;
	for (I=1; I<=6; I++) {
	DSEEN[I]=false;
	DLOC[I]=0;
	} /* end loop */
	MOVE(TROLL,0);
	MOVE(TROLL+100,0);
	MOVE(TROLL2,PLAC[TROLL]);
	MOVE(TROLL2+100,FIXD[TROLL]);
	JUGGLE(CHASM);
	if(PROP[BEAR] != 3)DSTROY(BEAR);
	PROP[CHAIN]=0;
	FIXED[CHAIN]=0;
	PROP[AXE]=0;
	FIXED[AXE]=0;
	RSPEAK(129);
	game.clock1= -1;
	game.closng=true;
	 goto L19999;

/*  Once he's panicked, and clock2 has run out, we come here to set up the
 *  storage room.  The room has two locs, hardwired as 115 (ne) and 116 (sw).
 *  At the ne end, we place empty bottles, a nursery of plants, a bed of
 *  oysters, a pile of lamps, rods with stars, sleeping dwarves, and him.  At
 *  the sw end we place grate over treasures, snake pit, covey of caged birds,
 *  more rods, and pillows.  A mirror stretches across one wall.  Many of the
 *  objects come from known locations and/or states (e.g. the snake is known to
 *  have been destroyed and needn't be carried away from its old "place"),
 *  making the various objects be handled differently.  We also drop all other
 *  objects he might be carrying (lest he have some which could cause trouble,
 *  such as the keys).  We describe the flash of light and trundle back. */

L11000: PROP[BOTTLE]=PUT(BOTTLE,115,1);
	PROP[PLANT]=PUT(PLANT,115,0);
	PROP[OYSTER]=PUT(OYSTER,115,0);
	OBJTXT[OYSTER]=3;
	PROP[LAMP]=PUT(LAMP,115,0);
	PROP[ROD]=PUT(ROD,115,0);
	PROP[DWARF]=PUT(DWARF,115,0);
	LOC=115;
	game.oldloc=115;
	game.newloc=115;

/*  Leave the grate with normal (non-negative) property.  Reuse sign. */

	I=PUT(GRATE,116,0);
	I=PUT(SIGN,116,0);
	OBJTXT[SIGN]=OBJTXT[SIGN]+1;
	PROP[SNAKE]=PUT(SNAKE,116,1);
	PROP[BIRD]=PUT(BIRD,116,1);
	PROP[CAGE]=PUT(CAGE,116,0);
	PROP[ROD2]=PUT(ROD2,116,0);
	PROP[PILLOW]=PUT(PILLOW,116,0);

	PROP[MIRROR]=PUT(MIRROR,115,0);
	FIXED[MIRROR]=116;

	for (I=1; I<=100; I++) {
	if(TOTING(I))DSTROY(I);
	} /* end loop */

	RSPEAK(132);
	game.closed=true;
	 return;

/*  Another way we can force an end to things is by having the lamp give out.
 *  When it gets close, we come here to warn him.  We go to 12000 if the lamp
 *  and fresh batteries are here, in which case we replace the batteries and
 *  continue.  12200 is for other cases of lamp dying.  12400 is when it goes
 *  out.  Even then, he can explore outside for a while if desired. */

L12000: RSPEAK(188);
	PROP[BATTER]=1;
	if(TOTING(BATTER))DROP(BATTER,LOC);
	game.limit=game.limit+2500;
	game.lmwarn=false;
	 goto L19999;

L12200: if(game.lmwarn || !HERE(LAMP)) goto L19999;
	game.lmwarn=true;
	game.spk=187;
	if(PLACE[BATTER] == 0)game.spk=183;
	if(PROP[BATTER] == 1)game.spk=189;
	RSPEAK(game.spk);
	 goto L19999;

L12400: game.limit= -1;
	PROP[LAMP]=0;
	if(HERE(LAMP))RSPEAK(184);
	 goto L19999;

/*  Oh dear, he's disturbed the dwarves. */

L18999: RSPEAK(game.spk);
L19000: RSPEAK(136);
	score(0);
}
