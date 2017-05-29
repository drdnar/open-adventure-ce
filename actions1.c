#include <stdlib.h>
#include <stdbool.h>
#include "misc.h"
#include "main.h"
#include "share.h"
#include "funcs.h"
#include "database.h"

/* This stuff was broken off as part of an effort to get the main program
 * to compile without running out of memory.  We're called with a number
 * that says what label the caller wanted to "goto", and we return a
 * similar label number for the caller to "goto".
 */

/*  Analyse a verb.  Remember what it was, go back for object if second word
 *  unless verb is "say", which snarfs arbitrary second word. */

int action(FILE *input, long STARTAT) {
	switch(STARTAT) {
	   case 4000: goto L4000;
	   case 4090: goto L4090;
	   case 5000: goto L5000;
	   }
	BUG(99);

L4000:	VERB=K;
	SPK=ACTSPK[VERB];
	if(WD2 > 0 && VERB != SAY) return(2800);
	if(VERB == SAY)OBJ=WD2;
	if(OBJ > 0) goto L4090;

/*  Analyse an intransitive verb (ie, no object given yet). */

	switch (VERB-1) {
		case 0: goto L8010;	/* CARRY */
		case 1: return(8000);	/* DROP */
		case 2:	return(8000);	/* SAY */
		case 3: goto L8040;	/* UNLOC */
		case 4: return(2009);	/* NOTHI */
		case 5:	goto L8040;	/* LOCK */
		case 6: goto L8070;	/* LIGHT */
		case 7: goto L8080;	/* EXTIN */
		case 8: return(8000);	/* WAVE */
		case 9: return(8000);	/* CALM */
		case 10: return(2011);	/* WALK */
		case 11: goto L9120;	/* ATTAC */
		case 12: goto L9130;	/* POUR */
		case 13: goto L8140;	/* EAT */
		case 14: goto L9150;	/* DRINK */
		case 15: return(8000);	/* RUB */
		case 16: return(8000);	/* TOSS */
		case 17: goto L8180;	/* QUIT */
		case 18: return(8000);	/* FIND */
		case 19: goto L8200;	/* INVEN */
		case 20: return(8000);	/* FEED */
		case 21: goto L9220;	/* FILL */
		case 22: goto L9230;	/* BLAST */
		case 23: goto L8240;	/* SCOR */
		case 24: goto L8250;	/* FOO */
		case 25: goto L8260;	/* BRIEF */
		case 26: goto L8270;	/* READ */
		case 27: return(8000);	/* BREAK */
		case 28: return(8000);	/* WAKE */
		case 29: goto L8300;	/* SUSP */
		case 30: goto L8310;	/* RESU */
		case 31: goto L8320;	/* FLY */
		case 32: goto L8330;	/* LISTEN */
		case 33: goto L8340;	/* ZZZZ */
	}
	BUG(23);

/*  Analyse a transitive verb. */

L4090:	switch (VERB-1) {
		case 0: goto L9010;	/* CARRY */
		case 1: goto L9020;	/* DROP */
		case 2: goto L9030;	/* SAY */
		case 3: goto L9040;	/* UNLOC */
		case 4: return(2009);	/* NOTHI */
		case 5: goto L9040;	/* LOCK */
		case 6: goto L9070;	/* LIGHT */
		case 7: goto L9080;	/* EXTI */
		case 8: goto L9090;	/* WAVE */
		case 9: return(2011);	/* CALM */
		case 10: return(2011);	/* WALK */
		case 11: goto L9120;	/* ATTAC */
		case 12: goto L9130;	/* POUR */
		case 13: goto L9140;	/* EAT */
		case 14: goto L9150;	/* DRINK */
		case 15: goto L9160;	/* RUB */
		case 16: goto L9170;	/* TOSS */
		case 17: return(2011);	/* QUIT */
		case 18: goto L9190;	/* FIND */
		case 19: goto L9190;	/* INVEN */
		case 20: goto L9210;	/* FEED */
		case 21: goto L9220;	/* FILL */
		case 22: goto L9230;	/* BLAST */
		case 23: return(2011);	/* SCOR */
		case 24: return(2011);	/* FOO */
		case 25: return(2011);	/* BRIEF */
		case 26: goto L9270;	/* READ */
		case 27: goto L9280;	/* BREAK */
		case 28: goto L9290;	/* WAKE */
		case 29: return(2011);	/* SUSP */
		case 30: return(2011);	/* RESU */
		case 31: goto L9320;	/* FLY */
		case 32: return(2011);	/* LISTEN */
		case 33: goto L8340;	/* ZZZZ */
	}
	BUG(24);

/*  Analyse an object word.  See if the thing is here, whether we've got a verb
 *  yet, and so on.  Object must be here unless verb is "find" or "invent(ory)"
 *  (and no new verb yet to be analysed).  Water and oil are also funny, since
 *  they are never actually dropped at any location, but might be here inside
 *  the bottle or urn or as a feature of the location. */

L5000:	OBJ=K;
	if(!HERE(K)) goto L5100;
L5010:	if(WD2 > 0) return(2800);
	if(VERB != 0) goto L4090;
	SETPRM(1,WD1,WD1X);
	RSPEAK(255);
	 return(2600);

L5100:	if(K != GRATE) goto L5110;
	if(LOC == 1 || LOC == 4 || LOC == 7)K=DPRSSN;
	if(LOC > 9 && LOC < 15)K=ENTRNC;
	if(K != GRATE) return(8);
L5110:	if(K == DWARF && ATDWRF(LOC) > 0) goto L5010;
	if((LIQ(0) == K && HERE(BOTTLE)) || K == LIQLOC(LOC)) goto L5010;
	if(OBJ != OIL || !HERE(URN) || PROP[URN] == 0) goto L5120;
	OBJ=URN;
	 goto L5010;
L5120:	if(OBJ != PLANT || !AT(PLANT2) || PROP[PLANT2] == 0) goto L5130;
	OBJ=PLANT2;
	 goto L5010;
L5130:	if(OBJ != KNIFE || KNFLOC != LOC) goto L5140;
	KNFLOC= -1;
	SPK=116;
	 return(2011);
L5140:	if(OBJ != ROD || !HERE(ROD2)) goto L5190;
	OBJ=ROD2;
	 goto L5010;
L5190:	if((VERB == FIND || VERB == INVENT) && WD2 <= 0) goto L5010;
	SETPRM(1,WD1,WD1X);
	RSPEAK(256);
	 return(2012);




/*  Routines for performing the various action verbs */

/*  Statement numbers in this section are 8000 for intransitive verbs, 9000 for
 *  transitive, plus ten times the verb number.  Many intransitive verbs use the
 *  transitive code, and some verbs use code for other verbs, as noted below. */

/*  Carry, no object given yet.  OK if only one object present. */

L8010:	if(ATLOC[LOC] == 0 || LINK[ATLOC[LOC]] != 0 || ATDWRF(LOC) > 0) return(8000);
	OBJ=ATLOC[LOC];

/*  Transitive carry/drop are in separate file. */

L9010:	return(carry());
L9020:	return(discard(false));

/*  SAY.  Echo WD2 (or WD1 if no WD2 (SAY WHAT?, etc.).)  Magic words override. */

L9030:	SETPRM(1,WD2,WD2X);
	if(WD2 <= 0)SETPRM(1,WD1,WD1X);
	if(WD2 > 0)WD1=WD2;
	I=VOCAB(WD1,-1);
	if(I == 62 || I == 65 || I == 71 || I == 2025 || I == 2034) goto L9035;
	RSPEAK(258);
	 return(2012);

L9035:	WD2=0;
	OBJ=0;
	 return(2630);

/*  Lock, unlock, no object given.  Assume various things if present. */

L8040:	SPK=28;
	if(HERE(CLAM))OBJ=CLAM;
	if(HERE(OYSTER))OBJ=OYSTER;
	if(AT(DOOR))OBJ=DOOR;
	if(AT(GRATE))OBJ=GRATE;
	if(OBJ != 0 && HERE(CHAIN)) return(8000);
	if(HERE(CHAIN))OBJ=CHAIN;
	if(OBJ == 0) return(2011);

/*  Lock, unlock object.  Special stuff for opening clam/oyster and for chain. */

L9040:	if(OBJ == CLAM || OBJ == OYSTER) goto L9046;
	if(OBJ == DOOR)SPK=111;
	if(OBJ == DOOR && PROP[DOOR] == 1)SPK=54;
	if(OBJ == CAGE)SPK=32;
	if(OBJ == KEYS)SPK=55;
	if(OBJ == GRATE || OBJ == CHAIN)SPK=31;
	if(SPK != 31 || !HERE(KEYS)) return(2011);
	if(OBJ == CHAIN) goto L9048;
	if(!CLOSNG) goto L9043;
	K=130;
	if(!PANIC)CLOCK2=15;
	PANIC=true;
	 return(2010);

L9043:	K=34+PROP[GRATE];
	PROP[GRATE]=1;
	if(VERB == LOCK)PROP[GRATE]=0;
	K=K+2*PROP[GRATE];
	 return(2010);

/*  Clam/Oyster. */
L9046:	K=0;
	if(OBJ == OYSTER)K=1;
	SPK=124+K;
	if(TOTING(OBJ))SPK=120+K;
	if(!TOTING(TRIDNT))SPK=122+K;
	if(VERB == LOCK)SPK=61;
	if(SPK != 124) return(2011);
	DSTROY(CLAM);
	DROP(OYSTER,LOC);
	DROP(PEARL,105);
	 return(2011);

/*  Chain. */
L9048:	if(VERB == LOCK) goto L9049;
	SPK=171;
	if(PROP[BEAR] == 0)SPK=41;
	if(PROP[CHAIN] == 0)SPK=37;
	if(SPK != 171) return(2011);
	PROP[CHAIN]=0;
	FIXED[CHAIN]=0;
	if(PROP[BEAR] != 3)PROP[BEAR]=2;
	FIXED[BEAR]=2-PROP[BEAR];
	 return(2011);

L9049:	SPK=172;
	if(PROP[CHAIN] != 0)SPK=34;
	if(LOC != PLAC[CHAIN])SPK=173;
	if(SPK != 172) return(2011);
	PROP[CHAIN]=2;
	if(TOTING(CHAIN))DROP(CHAIN,LOC);
	FIXED[CHAIN]= -1;
	 return(2011);

/*  Light.  Applicable only to lamp and urn. */

L8070:	if(HERE(LAMP) && PROP[LAMP] == 0 && LIMIT >= 0)OBJ=LAMP;
	if(HERE(URN) && PROP[URN] == 1)OBJ=OBJ*100+URN;
	if(OBJ == 0 || OBJ > 100) return(8000);

L9070:	if(OBJ == URN) goto L9073;
	if(OBJ != LAMP) return(2011);
	SPK=184;
	if(LIMIT < 0) return(2011);
	PROP[LAMP]=1;
	RSPEAK(39);
	if(WZDARK) return(2000);
	 return(2012);

L9073:	SPK=38;
	if(PROP[URN] == 0) return(2011);
	SPK=209;
	PROP[URN]=2;
	 return(2011);

/*  Extinguish.  Lamp, urn, dragon/volcano (nice try). */

L8080:	if(HERE(LAMP) && PROP[LAMP] == 1)OBJ=LAMP;
	if(HERE(URN) && PROP[URN] == 2)OBJ=OBJ*100+URN;
	if(OBJ == 0 || OBJ > 100) return(8000);

L9080:	if(OBJ == URN) goto L9083;
	if(OBJ == LAMP) goto L9086;
	if(OBJ == DRAGON || OBJ == VOLCAN)SPK=146;
	 return(2011);

L9083:	PROP[URN]=PROP[URN]/2;
	SPK=210;
	 return(2011);

L9086:	PROP[LAMP]=0;
	RSPEAK(40);
	if(DARK(0))RSPEAK(16);
	 return(2012);

/*  Wave.  No effect unless waving rod at fissure or at bird. */

L9090:	if((!TOTING(OBJ)) && (OBJ != ROD || !TOTING(ROD2)))SPK=29;
	if(OBJ != ROD || !TOTING(OBJ) || (!HERE(BIRD) && (CLOSNG || !AT(FISSUR))))
		return(2011);
	if(HERE(BIRD))SPK=206+MOD(PROP[BIRD],2);
	if(SPK == 206 && LOC == PLACE[STEPS] && PROP[JADE] < 0) goto L9094;
	if(CLOSED) return(18999);
	if(CLOSNG || !AT(FISSUR)) return(2011);
	if(HERE(BIRD))RSPEAK(SPK);
	PROP[FISSUR]=1-PROP[FISSUR];
	PSPEAK(FISSUR,2-PROP[FISSUR]);
	 return(2012);

L9094:	DROP(JADE,LOC);
	PROP[JADE]=0;
	TALLY=TALLY-1;
	SPK=208;
	 return(2011);

/*  Attack also moved into separate module. */

L9120:	return(attack(input));

/*  Pour.  If no object, or object is bottle, assume contents of bottle.
 *  special tests for pouring water or oil on plant or rusty door. */

L9130:	if(OBJ == BOTTLE || OBJ == 0)OBJ=LIQ(0);
	if(OBJ == 0) return(8000);
	if(!TOTING(OBJ)) return(2011);
	SPK=78;
	if(OBJ != OIL && OBJ != WATER) return(2011);
	if(HERE(URN) && PROP[URN] == 0) goto L9134;
	PROP[BOTTLE]=1;
	PLACE[OBJ]=0;
	SPK=77;
	if(!(AT(PLANT) || AT(DOOR))) return(2011);

	if(AT(DOOR)) goto L9132;
	SPK=112;
	if(OBJ != WATER) return(2011);
	PSPEAK(PLANT,PROP[PLANT]+3);
	PROP[PLANT]=MOD(PROP[PLANT]+1,3);
	PROP[PLANT2]=PROP[PLANT];
	K=NUL;
	 return(8);

L9132:	PROP[DOOR]=0;
	if(OBJ == OIL)PROP[DOOR]=1;
	SPK=113+PROP[DOOR];
	 return(2011);

L9134:	OBJ=URN;
	 goto L9220;

/*  Eat.  Intransitive: assume food if present, else ask what.  Transitive: food
 *  ok, some things lose appetite, rest are ridiculous. */

L8140:	if(!HERE(FOOD)) return(8000);
L8142:	DSTROY(FOOD);
	SPK=72;
	 return(2011);

L9140:	if(OBJ == FOOD) goto L8142;
	if(OBJ == BIRD || OBJ == SNAKE || OBJ == CLAM || OBJ == OYSTER || OBJ ==
		DWARF || OBJ == DRAGON || OBJ == TROLL || OBJ == BEAR || OBJ ==
		OGRE)SPK=71;
	 return(2011);

/*  Drink.  If no object, assume water and look for it here.  If water is in
 *  the bottle, drink that, else must be at a water loc, so drink stream. */

L9150:	if(OBJ == 0 && LIQLOC(LOC) != WATER && (LIQ(0) != WATER || !HERE(BOTTLE)))
		return(8000);
	if(OBJ == BLOOD) goto L9153;
	if(OBJ != 0 && OBJ != WATER)SPK=110;
	if(SPK == 110 || LIQ(0) != WATER || !HERE(BOTTLE)) return(2011);
	PROP[BOTTLE]=1;
	PLACE[WATER]=0;
	SPK=74;
	 return(2011);

L9153:	DSTROY(BLOOD);
	PROP[DRAGON]=2;
	OBJSND[BIRD]=OBJSND[BIRD]+3;
	SPK=240;
	 return(2011);

/*  Rub.  Yields various snide remarks except for lit urn. */

L9160:	if(OBJ != LAMP)SPK=76;
	if(OBJ != URN || PROP[URN] != 2) return(2011);
	DSTROY(URN);
	DROP(AMBER,LOC);
	PROP[AMBER]=1;
	TALLY=TALLY-1;
	DROP(CAVITY,LOC);
	SPK=216;
	 return(2011);

/*  Throw moved into separate module. */

L9170:	return(throw(input));

/*  Quit.  Intransitive only.  Verify intent and exit if that's what he wants. */

L8180:	if(YES(input,22,54,54)) score(1);
	 return(2012);

/*  Find.  Might be carrying it, or it might be here.  Else give caveat. */

L9190:	if(AT(OBJ) || (LIQ(0) == OBJ && AT(BOTTLE)) || K == LIQLOC(LOC) || (OBJ ==
		DWARF && ATDWRF(LOC) > 0))SPK=94;
	if(CLOSED)SPK=138;
	if(TOTING(OBJ))SPK=24;
	 return(2011);

/*  Inventory.  If object, treat same as find.  Else report on current burden. */

L8200:	SPK=98;
	/* 8201 */ for (I=1; I<=100; I++) {
	if(I == BEAR || !TOTING(I)) goto L8201;
	if(SPK == 98)RSPEAK(99);
	BLKLIN=false;
	PSPEAK(I,-1);
	BLKLIN=true;
	SPK=0;
L8201:	/*etc*/ ;
	} /* end loop */
	if(TOTING(BEAR))SPK=141;
	 return(2011);

/* Feed/fill are in the other module. */

L9210:	return(feed());
L9220:	return(fill());

/*  Blast.  No effect unless you've got dynamite, which is a neat trick! */

L9230:	if(PROP[ROD2] < 0 || !CLOSED) return(2011);
	BONUS=133;
	if(LOC == 115)BONUS=134;
	if(HERE(ROD2))BONUS=135;
	RSPEAK(BONUS);
	 score(0);

/*  Score.  Call scoring routine but tell it to return. */

L8240:	score(-1);
	SETPRM(1,SCORE,MXSCOR);
	SETPRM(3,TURNS,TURNS);
	RSPEAK(259);
	 return(2012);

/*  FEE FIE FOE FOO (AND FUM).  ADVANCE TO NEXT STATE IF GIVEN IN PROPER ORDER.
 *  LOOK UP WD1 IN SECTION 3 OF VOCAB TO DETERMINE WHICH WORD WE'VE GOT.  LAST
 *  WORD ZIPS THE EGGS BACK TO THE GIANT ROOM (UNLESS ALREADY THERE). */

L8250:	K=VOCAB(WD1,3);
	SPK=42;
	if(FOOBAR == 1-K) goto L8252;
	if(FOOBAR != 0)SPK=151;
	 return(2011);

L8252:	FOOBAR=K;
	if(K != 4) return(2009);
	FOOBAR=0;
	if(PLACE[EGGS] == PLAC[EGGS] || (TOTING(EGGS) && LOC == PLAC[EGGS])) 
		return(2011);
/*  Bring back troll if we steal the eggs back from him before crossing. */
	if(PLACE[EGGS] == 0 && PLACE[TROLL] == 0 && PROP[TROLL] ==
		0)PROP[TROLL]=1;
	K=2;
	if(HERE(EGGS))K=1;
	if(LOC == PLAC[EGGS])K=0;
	MOVE(EGGS,PLAC[EGGS]);
	PSPEAK(EGGS,K);
	 return(2012);

/*  Brief.  Intransitive only.  Suppress long descriptions after first time. */

L8260:	SPK=156;
	ABBNUM=10000;
	DETAIL=3;
	 return(2011);

/*  Read.  Print stuff based on objtxt.  Oyster (?) is special case. */

L8270:	for (I=1; I<=100; I++) {
	if(HERE(I) && OBJTXT[I] != 0 && PROP[I] >= 0)OBJ=OBJ*100+I;
	} /* end loop */
	if(OBJ > 100 || OBJ == 0 || DARK(0)) return(8000);

L9270:	if(DARK(0)) goto L5190;
	if(OBJTXT[OBJ] == 0 || PROP[OBJ] < 0) return(2011);
	if(OBJ == OYSTER && !CLSHNT) goto L9275;
	PSPEAK(OBJ,OBJTXT[OBJ]+PROP[OBJ]);
	 return(2012);

L9275:	CLSHNT=YES(input,192,193,54);
	 return(2012);

/*  Break.  Only works for mirror in repository and, of course, the vase. */

L9280:	if(OBJ == MIRROR)SPK=148;
	if(OBJ == VASE && PROP[VASE] == 0) goto L9282;
	if(OBJ != MIRROR || !CLOSED) return(2011);
	SPK=197;
	 return(18999);

L9282:	SPK=198;
	if(TOTING(VASE))DROP(VASE,LOC);
	PROP[VASE]=2;
	FIXED[VASE]= -1;
	 return(2011);

/*  Wake.  Only use is to disturb the dwarves. */

L9290:	if(OBJ != DWARF || !CLOSED) return(2011);
	SPK=199;
	 return(18999);

/*  Suspend.  Offer to save things in a file, but charging some points (so
 *  can't win by using saved games to retry battles or to start over after
 *  learning zzword). */

L8300:	SPK=201;
	RSPEAK(260);
	if(!YES(input,200,54,54)) return(2012);
	SAVED=SAVED+5;
	KK= -1;

/*  This next part is shared with the "resume" code.  The two cases are
 *  distinguished by the value of kk (-1 for suspend, +1 for resume). */

L8305:	DATIME(&I,&K);
	K=I+650*K;
	SAVWRD(KK,K);
	K=VRSION;
	SAVWRD(0,K);
	if(K != VRSION) goto L8312;
/*  Herewith are all the variables whose values can change during a game,
 *  omitting a few (such as I, J, ATTACK) whose values between turns are
 *  irrelevant and some whose values when a game is
 *  suspended or resumed are guaranteed to match.  If unsure whether a value
 *  needs to be saved, include it.  Overkill can't hurt.  Pad the last savwds
 *  with junk variables to bring it up to 7 values. */
	SAVWDS(ABBNUM,BLKLIN,BONUS,CLOCK1,CLOCK2,CLOSED,CLOSNG);
	SAVWDS(DETAIL,DFLAG,DKILL,DTOTAL,FOOBAR,HOLDNG,IWEST);
	SAVWDS(KNFLOC,LIMIT,LL,LMWARN,LOC,NEWLOC,NUMDIE);
	SAVWDS(OBJ,OLDLC2,OLDLOC,OLDOBJ,PANIC,SAVED,SETUP);
	SAVWDS(SPK,TALLY,THRESH,TRNDEX,TRNLUZ,TURNS,OBJTXT[OYSTER]);
	SAVWDS(VERB,WD1,WD1X,WD2,WZDARK,ZZWORD,OBJSND[BIRD]);
	SAVWDS(OBJTXT[SIGN],CLSHNT,NOVICE,K,K,K,K);
	SAVARR(ABB,LOCSIZ);
	SAVARR(ATLOC,LOCSIZ);
	SAVARR(DLOC,6);
	SAVARR(DSEEN,6);
	SAVARR(FIXED,100);
	SAVARR(HINTED,HNTSIZ);
	SAVARR(HINTLC,HNTSIZ);
	SAVARR(LINK,200);
	SAVARR(ODLOC,6);
	SAVARR(PLACE,100);
	SAVARR(PROP,100);
	SAVWRD(KK,K);
	if(K != 0) goto L8318;
	K=NUL;
	ZZWORD=RNDVOC(3,ZZWORD);
	if(KK > 0) return(8);
	RSPEAK(266);
	exit(0);

/*  Resume.  Read a suspended game back from a file. */

L8310:	KK=1;
	if(LOC == 1 && ABB[1] == 1) goto L8305;
	RSPEAK(268);
	if(!YES(input,200,54,54)) return(2012);
	 goto L8305;

L8312:	SETPRM(1,K/10,MOD(K,10));
	SETPRM(3,VRSION/10,MOD(VRSION,10));
	RSPEAK(269);
	 return(2000);

L8318:	RSPEAK(270);
	exit(0);

/*  Fly.  Snide remarks unless hovering rug is here. */

L8320:	if(PROP[RUG] != 2)SPK=224;
	if(!HERE(RUG))SPK=225;
	if(SPK/2 == 112) return(2011);
	OBJ=RUG;

L9320:	if(OBJ != RUG) return(2011);
	SPK=223;
	if(PROP[RUG] != 2) return(2011);
	OLDLC2=OLDLOC;
	OLDLOC=LOC;
	NEWLOC=PLACE[RUG]+FIXED[RUG]-LOC;
	SPK=226;
	if(PROP[SAPPH] >= 0)SPK=227;
	RSPEAK(SPK);
	 return(2);

/*  Listen.  Intransitive only.  Print stuff based on objsnd/locsnd. */

L8330:	SPK=228;
	K=LOCSND[LOC];
	if(K == 0) goto L8332;
	RSPEAK(IABS(K));
	if(K < 0) return(2012);
	SPK=0;
L8332:	SETPRM(1,ZZWORD,0);
	/* 8335 */ for (I=1; I<=100; I++) {
	if(!HERE(I) || OBJSND[I] == 0 || PROP[I] < 0) goto L8335;
	PSPEAK(I,OBJSND[I]+PROP[I]);
	SPK=0;
	if(I == BIRD && OBJSND[I]+PROP[I] == 8)DSTROY(BIRD);
L8335:	/*etc*/ ;
	} /* end loop */
	 return(2011);

/*  Z'ZZZ (word gets recomputed at startup; different each game). */

L8340:	if(!AT(RESER) && LOC != FIXED[RESER]-1) return(2011);
	PSPEAK(RESER,PROP[RESER]+1);
	PROP[RESER]=1-PROP[RESER];
	if(AT(RESER)) return(2012);
	OLDLC2=LOC;
	NEWLOC=0;
	RSPEAK(241);
	 return(2);

}
