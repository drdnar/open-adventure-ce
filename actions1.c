#include <stdlib.h>
#include <stdbool.h>
#include "advent.h"
#include "database.h"

/*
 * Action handlers.  Eventually we'll do lookup through a method table
 * that calls these.  Absolutely nothing like the original FORTRAN.
 */

static int bigwords(long foo)
/*  FEE FIE FOE FOO (AND FUM).  Advance to next state if given in proper order.
 *  Look up WD1 in section 3 of vocab to determine which word we've got.  Last
 *  word zips the eggs back to the giant room (unless already there). */
{
    int k=VOCAB(foo,3);
    SPK=42;
    if(game.foobar != 1-k) {
	if(game.foobar != 0)SPK=151;
	return(2011);
    } else {

	game.foobar=k;
	if(k != 4) return(2009);
	game.foobar=0;
	if(game.place[EGGS] == PLAC[EGGS] || (TOTING(EGGS) && game.loc == PLAC[EGGS])) 
	    return(2011);
	/*  Bring back troll if we steal the eggs back from him before
	 *  crossing. */
	if(game.place[EGGS]==0 && game.place[TROLL]==0 && game.prop[TROLL]==0)
	    game.prop[TROLL]=1;
	k=2;
	if(HERE(EGGS))k=1;
	if(game.loc == PLAC[EGGS])k=0;
	MOVE(EGGS,PLAC[EGGS]);
	PSPEAK(EGGS,k);
	return(2012);
    }
}

static int bivalve(token_t verb, token_t obj)
/* Clam/oyster actions */
{
    int k=0;
    if(obj == OYSTER)k=1;
    SPK=124+k;
    if(TOTING(obj))SPK=120+k;
    if(!TOTING(TRIDNT))SPK=122+k;
    if(verb == LOCK)SPK=61;
    if(SPK != 124)
	return(2011);
    DSTROY(CLAM);
    DROP(OYSTER,game.loc);
    DROP(PEARL,105);
    return(2011);
}

static int blast(void)
/*  Blast.  No effect unless you've got dynamite, which is a neat trick! */
{
    if(game.prop[ROD2] < 0 || !game.closed) return(2011);
    game.bonus=133;
    if(game.loc == 115)game.bonus=134;
    if(HERE(ROD2))game.bonus=135;
    RSPEAK(game.bonus);
    score(0);
}

static int vbreak(token_t obj)
/*  Break.  Only works for mirror in repository and, of course, the vase. */
{
    if(obj == MIRROR)SPK=148;
    if(obj == VASE && game.prop[VASE] == 0) {
	SPK=198;
	if(TOTING(VASE))DROP(VASE,game.loc);
	game.prop[VASE]=2;
	game.fixed[VASE]= -1;
	return(2011);
    } else {
	if(obj != MIRROR || !game.closed) return(2011);
	SPK=197;
	return(18999);
    }
}


static int brief(void)
/*  Brief.  Intransitive only.  Suppress long descriptions after first time. */
{
    SPK=156;
    game.abbnum=10000;
    game.detail=3;
    return(2011);
}

static int chain(token_t verb)
/* Do something to the bear's chain */
{
    if(verb != LOCK) {
	SPK=171;
	if(game.prop[BEAR] == 0)SPK=41;
	if(game.prop[CHAIN] == 0)SPK=37;
	if(SPK != 171) return(2011);
	game.prop[CHAIN]=0;
	game.fixed[CHAIN]=0;
	if(game.prop[BEAR] != 3)game.prop[BEAR]=2;
	game.fixed[BEAR]=2-game.prop[BEAR];
	return(2011);
    } else {
	SPK=172;
	if(game.prop[CHAIN] != 0)SPK=34;
	if(game.loc != PLAC[CHAIN])SPK=173;
	if(SPK != 172) return(2011);
	game.prop[CHAIN]=2;
	if(TOTING(CHAIN))DROP(CHAIN,game.loc);
	game.fixed[CHAIN]= -1;
	return(2011);
    }
}

static int drink(token_t obj)
/*  Drink.  If no object, assume water and look for it here.  If water is in
 *  the bottle, drink that, else must be at a water loc, so drink stream. */
{
    if(obj == 0 && LIQLOC(game.loc) != WATER && (LIQ(0) != WATER || !HERE(BOTTLE)))
	return(8000);
    if(obj != BLOOD) {
	if(obj != 0 && obj != WATER)SPK=110;
	if(SPK == 110 || LIQ(0) != WATER || !HERE(BOTTLE)) return(2011);
	game.prop[BOTTLE]=1;
	game.place[WATER]=0;
	SPK=74;
	return(2011);
    } else {
	DSTROY(BLOOD);
	game.prop[DRAGON]=2;
	OBJSND[BIRD]=OBJSND[BIRD]+3;
	SPK=240;
	return(2011);
    }
}

static int extinguish(int obj)
/* Extinguish.  Lamp, urn, dragon/volcano (nice try). */
{
    if (obj == INTRANSITIVE) {
	if(HERE(LAMP) && game.prop[LAMP] == 1)obj=LAMP;
	if(HERE(URN) && game.prop[URN] == 2)obj=obj*NOBJECTS+URN;
	if(obj == 0 || obj > NOBJECTS) return(8000);
    }

    if(obj == URN) {
	game.prop[URN]=game.prop[URN]/2;
	SPK=210;
	return(2011);
    }
    if(obj == LAMP) {
	game.prop[LAMP]=0;
	RSPEAK(40);
	if(DARK(0))
	    RSPEAK(16);
	return(2012);
    }
    if(obj == DRAGON || obj == VOLCAN)
	SPK=146;
    return(2011);
}

static int find(token_t obj)
/* Find.  Might be carrying it, or it might be here.  Else give caveat. */
{
    if(AT(obj) ||
       (LIQ(0) == obj && AT(BOTTLE)) ||
       K == LIQLOC(game.loc) ||
       (obj == DWARF && ATDWRF(game.loc) > 0))
	SPK=94;
    if(game.closed)SPK=138;
    if(TOTING(obj))SPK=24;
    return(2011);
}

static int inven(token_t obj)
/* Inventory. If object, treat same as find.  Else report on current burden. */
{
    int i;
    SPK=98;
    for (i=1; i<=NOBJECTS; i++) {
	if(i == BEAR || !TOTING(i))
	    continue;
	if(SPK == 98)RSPEAK(99);
	game.blklin=false;
	PSPEAK(i,-1);
	game.blklin=true;
	SPK=0;
    }
    if(TOTING(BEAR))
	SPK=141;
    return(2011);
}

int light(token_t obj)
/*  Light.  Applicable only to lamp and urn. */
{
    if (obj == INTRANSITIVE) {
	if(HERE(LAMP) && game.prop[LAMP] == 0 && game.limit >= 0)obj=LAMP;
	if(HERE(URN) && game.prop[URN] == 1)obj=obj*NOBJECTS+URN;
	if(obj == 0 || obj > NOBJECTS) return(8000);
    }

    if (obj == URN) {
	SPK=38;
	if(game.prop[URN] == 0)
	    return(2011);
	SPK=209;
	game.prop[URN]=2;
	return(2011);
    } else {
	if(obj != LAMP) return(2011);
	SPK=184;
	if(game.limit < 0) return(2011);
	game.prop[LAMP]=1;
	RSPEAK(39);
	if(game.wzdark) return(2000);
	return(2012);
    }	 
}

static int listen(void)
/*  Listen.  Intransitive only.  Print stuff based on objsnd/locsnd. */
{
    int i, k;
    SPK=228;
    k=LOCSND[game.loc];
    if(k != 0) {
	RSPEAK(labs(k));
	if(k < 0) return(2012);
	SPK=0;
    }
    SETPRM(1,game.zzword,0);
    for (i=1; i<=NOBJECTS; i++) {
	if(!HERE(i) || OBJSND[i] == 0 || game.prop[i] < 0)
	    continue;
	PSPEAK(i,OBJSND[i]+game.prop[i]);
	SPK=0;
	if(i == BIRD && OBJSND[i]+game.prop[i] == 8)
	    DSTROY(BIRD);
    }
    return(2011);
}

static int lock(token_t verb, token_t obj)
/* Lock, unlock, no object given.  Assume various things if present. */
{
    int k;
    if (obj == INTRANSITIVE) {
	SPK=28;
	if(HERE(CLAM))obj=CLAM;
	if(HERE(OYSTER))obj=OYSTER;
	if(AT(DOOR))obj=DOOR;
	if(AT(GRATE))obj=GRATE;
	if(obj != 0 && HERE(CHAIN)) return(8000);
	if(HERE(CHAIN))obj=CHAIN;
	if(obj == 0) return(2011);
    }
	
    /*  Lock, unlock object.  Special stuff for opening clam/oyster
     *  and for chain. */
    if(obj == CLAM || obj == OYSTER)
	return bivalve(verb, obj);
    if(obj == DOOR)SPK=111;
    if(obj == DOOR && game.prop[DOOR] == 1)SPK=54;
    if(obj == CAGE)SPK=32;
    if(obj == KEYS)SPK=55;
    if(obj == GRATE || obj == CHAIN)SPK=31;
    if(SPK != 31 || !HERE(KEYS)) return(2011);
    if(obj == CHAIN)
	return chain(verb);
    if (game.closng) {
	SPK=130;
	if(!game.panic)game.clock2=15;
	game.panic=true;
	return(2011);
    }
    SPK=34+game.prop[GRATE];
    game.prop[GRATE]=1;
    if(verb == LOCK)game.prop[GRATE]=0;
    SPK=SPK+2*game.prop[GRATE];
    return(2011);
}

static int pour(token_t obj)
/*  Pour.  If no object, or object is bottle, assume contents of bottle.
 *  special tests for pouring water or oil on plant or rusty door. */
{
    if(obj == BOTTLE || obj == 0)obj=LIQ(0);
    if(obj == 0) return(8000);
    if(!TOTING(obj)) return(2011);
    SPK=78;
    if(obj != OIL && obj != WATER) return(2011);
    if(HERE(URN) && game.prop[URN] == 0)
	return fill(URN);
    game.prop[BOTTLE]=1;
    game.place[obj]=0;
    SPK=77;
    if(!(AT(PLANT) || AT(DOOR)))
	return(2011);
    if(!AT(DOOR)) {
	SPK=112;
	if(obj != WATER) return(2011);
	PSPEAK(PLANT,game.prop[PLANT]+3);
	game.prop[PLANT]=MOD(game.prop[PLANT]+1,3);
	game.prop[PLANT2]=game.prop[PLANT];
	K=NUL;
	return(8);
    } else {
	game.prop[DOOR]=0;
	if(obj == OIL)game.prop[DOOR]=1;
	SPK=113+game.prop[DOOR];
	return(2011);
    }
}

static int quit(FILE *input)
/*  Quit.  Intransitive only.  Verify intent and exit if that's what he wants. */
{
    if(YES(input,22,54,54))
	score(1);
    return(2012);
}

static int rub(token_t obj)
/* Rub.  Yields various snide remarks except for lit urn. */
{
    if(obj != LAMP)SPK=76;
    if(obj != URN || game.prop[URN] != 2) return(2011);
    DSTROY(URN);
    DROP(AMBER,game.loc);
    game.prop[AMBER]=1;
    game.tally=game.tally-1;
    DROP(CAVITY,game.loc);
    SPK=216;
    return(2011);
}

static int say(void)
/* SAY.  Echo WD2 (or WD1 if no WD2 (SAY WHAT?, etc.).)  Magic words override. */
{
    /* FIXME: ugly use of globals */
    SETPRM(1,WD2,WD2X);
    if(WD2 <= 0)SETPRM(1,WD1,WD1X);
    if(WD2 > 0)WD1=WD2;
    I=VOCAB(WD1,-1);
    if(I == 62 || I == 65 || I == 71 || I == 2025 || I == 2034) goto L9035;
    RSPEAK(258);
    return(2012);

L9035:	WD2=0;
    //obj=0;
    return(2630);
}

static int vscore(void)
/* Score.  Call scoring routine but tell it to return. */
{
    score(-1);
    return(2012);
}

static int wake(token_t obj)
/* Wake.  Only use is to disturb the dwarves. */
{
    if(obj != DWARF || !game.closed) return(2011);
    SPK=199;
    return(18999);
}

static int wave(token_t obj)
/* Wave.  No effect unless waving rod at fissure or at bird. */
{
    if((!TOTING(obj)) && (obj != ROD || !TOTING(ROD2)))SPK=29;
    if(obj != ROD ||
       !TOTING(obj) ||
       (!HERE(BIRD) && (game.closng || !AT(FISSUR))))
	return(2011);
    if(HERE(BIRD))SPK=206+MOD(game.prop[BIRD],2);
    if(SPK == 206 && game.loc == game.place[STEPS] && game.prop[JADE] < 0) {
	DROP(JADE,game.loc);
	game.prop[JADE]=0;
	game.tally=game.tally-1;
	SPK=208;
	return(2011);
    } else {
	if(game.closed) return(18999);
	if(game.closng || !AT(FISSUR)) return(2011);
	if(HERE(BIRD))RSPEAK(SPK);
	game.prop[FISSUR]=1-game.prop[FISSUR];
	PSPEAK(FISSUR,2-game.prop[FISSUR]);
	return(2012);
    }
}

/* This stuff was broken off as part of an effort to get the main program
 * to compile without running out of memory.  We're called with a number
 * that says what label the caller wanted to "goto", and we return a
 * similar label number for the caller to "goto".
 */

/*  Analyse a verb.  Remember what it was, go back for object if second word
 *  unless verb is "say", which snarfs arbitrary second word.
 */

int action(FILE *input, long STARTAT, long verb, long obj) {
	int kk;
	switch(STARTAT) {
	   case 4000: goto L4000;
	   case 4090: goto L4090;
	   case 5000: goto L5000;
	   }
	BUG(99);

L4000:	
	SPK=ACTSPK[verb];
	if(WD2 > 0 && verb != SAY) return(2800);
	if(verb == SAY)obj=WD2;
	if(obj > 0) goto L4090;

/*  Analyse an intransitive verb (ie, no object given yet). */

	switch (verb-1) {
		case  0: /* CARRY */ goto L8010;
		case  1: /* DROP  */ return(8000); 
		case  2: /* SAY   */ return(8000); 
		case  3: /* UNLOC */ goto L8040;    
		case  4: /* NOTHI */ return(2009); 
		case  5: /* LOCK  */ goto L8040;    
		case  6: /* LIGHT */ goto L8070;    
		case  7: /* EXTIN */ goto L8080;    
		case  8: /* WAVE  */ return(8000); 
		case  9: /* CALM  */ return(8000); 
		case 10: /* WALK  */ return(2011); 
		case 11: /* ATTAC */ goto L9120;   
		case 12: /* POUR  */ goto L9130;   
		case 13: /* EAT   */ goto L8140;   
		case 14: /* DRINK */ goto L9150;   
		case 15: /* RUB   */ return(8000); 
		case 16: /* TOSS  */ return(8000); 
		case 17: /* QUIT  */ goto L8180;   
		case 18: /* FIND  */ return(8000); 
		case 19: /* INVEN */ goto L8200;   
		case 20: /* FEED  */ return(8000); 
		case 21: /* FILL  */ goto L9220;   
		case 22: /* BLAST */ goto L9230;   
		case 23: /* SCOR  */ goto L8240;   
		case 24: /* FOO   */ goto L8250;   
		case 25: /* BRIEF */ goto L8260;   
		case 26: /* READ  */ goto L8270;   
		case 27: /* BREAK */ return(8000); 
		case 28: /* WAKE  */ return(8000); 
		case 29: /* SUSP  */ goto L8300;   
		case 30: /* RESU  */ goto L8310;   
		case 31: /* FLY   */ goto L8320;   
		case 32: /* LISTE */ goto L8330;   
		case 33: /* ZZZZ  */ goto L8340;   
	}
	BUG(23);

/*  Analyse a transitive verb. */

L4090:	switch (verb-1) {
		case  0: /* CARRY */ goto L9010;    
		case  1: /* DROP  */ goto L9020;    
		case  2: /* SAY   */ goto L9030;    
		case  3: /* UNLOC */ goto L9040;    
		case  4: /* NOTHI */ return(2009); 
		case  5: /* LOCK  */ goto L9040;    
		case  6: /* LIGHT */ goto L9070;    
		case  7: /* EXTI  */ goto L9080;    
		case  8: /* WAVE  */ goto L9090;    
		case  9: /* CALM  */ return(2011); 
		case 10: /* WALK  */ return(2011); 
		case 11: /* ATTAC */ goto L9120;   
		case 12: /* POUR  */ goto L9130;   
		case 13: /* EAT   */ goto L9140;   
		case 14: /* DRINK */ goto L9150;   
		case 15: /* RUB   */ goto L9160;   
		case 16: /* TOSS  */ goto L9170;   
		case 17: /* QUIT  */ return(2011); 
		case 18: /* FIND  */ goto L9190;   
		case 19: /* INVEN */ goto L9190;   
		case 20: /* FEED  */ goto L9210;   
		case 21: /* FILL  */ goto L9220;   
		case 22: /* BLAST */ goto L9230;   
		case 23: /* SCOR  */ return(2011); 
		case 24: /* FOO   */ return(2011); 
		case 25: /* BRIEF */ return(2011); 
		case 26: /* READ  */ goto L9270;   
		case 27: /* BREAK */ goto L9280;   
		case 28: /* WAKE  */ goto L9290;   
		case 29: /* SUSP  */ return(2011); 
		case 30: /* RESU  */ return(2011); 
		case 31: /* FLY   */ goto L9320;   
		case 32: /* LISTE */ return(2011); 
		case 33: /* ZZZZ  */ goto L8340;   
	}
	BUG(24);

/*  Analyse an object word.  See if the thing is here, whether we've got a verb
 *  yet, and so on.  Object must be here unless verb is "find" or "invent(ory)"
 *  (and no new verb yet to be analysed).  Water and oil are also funny, since
 *  they are never actually dropped at any location, but might be here inside
 *  the bottle or urn or as a feature of the location. */

L5000:	obj=K;
	if(!HERE(K)) goto L5100;
L5010:	if(WD2 > 0) return(2800);
	if(verb != 0) goto L4090;
	SETPRM(1,WD1,WD1X);
	RSPEAK(255);
	 return(2600);

L5100:	if(K != GRATE) goto L5110;
	if(game.loc == 1 || game.loc == 4 || game.loc == 7)K=DPRSSN;
	if(game.loc > 9 && game.loc < 15)K=ENTRNC;
	if(K != GRATE) return(8);
L5110:	if(K == DWARF && ATDWRF(game.loc) > 0) goto L5010;
	if((LIQ(0) == K && HERE(BOTTLE)) || K == LIQLOC(game.loc)) goto L5010;
	if(obj != OIL || !HERE(URN) || game.prop[URN] == 0) goto L5120;
	obj=URN;
	 goto L5010;
L5120:	if(obj != PLANT || !AT(PLANT2) || game.prop[PLANT2] == 0) goto L5130;
	obj=PLANT2;
	 goto L5010;
L5130:	if(obj != KNIFE || game.knfloc != game.loc) goto L5140;
	game.knfloc= -1;
	SPK=116;
	 return(2011);
L5140:	if(obj != ROD || !HERE(ROD2)) goto L5190;
	obj=ROD2;
	 goto L5010;
L5190:	if((verb == FIND || verb == INVENT) && WD2 <= 0) goto L5010;
	SETPRM(1,WD1,WD1X);
	RSPEAK(256);
	 return(2012);




/*  Routines for performing the various action verbs */

/*  Statement numbers in this section are 8000 for intransitive verbs, 9000 for
 *  transitive, plus ten times the verb number.  Many intransitive verbs use the
 *  transitive code, and some verbs use code for other verbs, as noted below. */

L8010: return carry(INTRANSITIVE);
L9010: return carry(obj);
L9020: return discard(obj, false);
L9030: return say();
L8040: return lock(verb, INTRANSITIVE);
L9040: return lock(verb, obj);
L9046: return bivalve(verb, obj);
L9048: return chain(verb);
L8070: return light(INTRANSITIVE);
L9070: return light(obj);
L8080: return extinguish(INTRANSITIVE);
L9080: return extinguish(obj);
L9090: return wave(obj);
L9120: return attack(input, verb, obj);
L9130: return pour(obj);

/*  Eat.  Intransitive: assume food if present, else ask what.  Transitive: food
 *  ok, some things lose appetite, rest are ridiculous. */

L8140:	if(!HERE(FOOD)) return(8000);
L8142:	DSTROY(FOOD);
	SPK=72;
	 return(2011);

L9140:	if(obj == FOOD) goto L8142;
	if(obj == BIRD || obj == SNAKE || obj == CLAM || obj == OYSTER || obj ==
		DWARF || obj == DRAGON || obj == TROLL || obj == BEAR || obj ==
		OGRE)SPK=71;
	 return(2011);

L9150: return drink(obj);
L9160: return rub(obj);
L9170: return throw(input, verb, obj);
L8180: return quit(input);
L9190: return find(obj);
L8200: return inven(obj);
L9210: return feed(obj);
L9220: return fill(obj);
L9230: return blast();
L8240: return vscore();
L8250: return bigwords(WD1);
L8260: return brief();

/*  Read.  Print stuff based on objtxt.  Oyster (?) is special case. */

L8270:	for (I=1; I<=NOBJECTS; I++) {
	if(HERE(I) && OBJTXT[I] != 0 && game.prop[I] >= 0)obj=obj*NOBJECTS+I;
	} /* end loop */
	if(obj > NOBJECTS || obj == 0 || DARK(0)) return(8000);

L9270:	if(DARK(0)) goto L5190;
	if(OBJTXT[obj] == 0 || game.prop[obj] < 0) return(2011);
	if(obj == OYSTER && !game.clshnt) goto L9275;
	PSPEAK(obj,OBJTXT[obj]+game.prop[obj]);
	 return(2012);

L9275:	game.clshnt=YES(input,192,193,54);
	 return(2012);

L9280: return vbreak(obj);

L9290: return wake(obj);

/*  Suspend.  Offer to save things in a file, but charging some points (so
 *  can't win by using saved games to retry battles or to start over after
 *  learning zzword). */

L8300:	SPK=201;
	RSPEAK(260);
	if(!YES(input,200,54,54)) return(2012);
	game.saved=game.saved+5;
	kk= -1;

/*  This next part is shared with the "resume" code.  The two cases are
 *  distinguished by the value of kk (-1 for suspend, +1 for resume). */

L8305:	DATIME(&I,&K);
	K=I+650*K;
	SAVWRD(kk,K);
	K=VRSION;
	SAVWRD(0,K);
	if(K != VRSION) goto L8312;
/*  Herewith are all the variables whose values can change during a game,
 *  omitting a few (such as I, J, ATTACK) whose values between turns are
 *  irrelevant and some whose values when a game is
 *  suspended or resumed are guaranteed to match.  If unsure whether a value
 *  needs to be saved, include it.  Overkill can't hurt.  Pad the last savwds
 *  with junk variables to bring it up to 7 values. */
	SAVWDS(game.abbnum,game.blklin,game.bonus,game.clock1,game.clock2,game.closed,game.closng);
	SAVWDS(game.detail,game.dflag,game.dkill,game.dtotal,game.foobar,game.holdng,game.iwest);
	SAVWDS(game.knfloc,game.limit,K,game.lmwarn,game.loc,game.newloc,game.numdie);
	SAVWDS(K,game.oldlc2,game.oldloc,game.oldobj,game.panic,game.saved,game.setup);
	SAVWDS(SPK,game.tally,game.thresh,game.trndex,game.trnluz,game.turns,OBJTXT[OYSTER]);
	SAVWDS(K,WD1,WD1X,WD2,game.wzdark,game.zzword,OBJSND[BIRD]);
	SAVWDS(OBJTXT[SIGN],game.clshnt,game.novice,K,K,K,K);
	SAVARR(game.abbrev,LOCSIZ);
	SAVARR(game.atloc,LOCSIZ);
	SAVARR(game.dloc,NDWARVES);
	SAVARR(game.dseen,NDWARVES);
	SAVARR(game.fixed,NOBJECTS);
	SAVARR(game.hinted,HNTSIZ);
	SAVARR(game.hintlc,HNTSIZ);
	SAVARR(game.link,NOBJECTS*2);
	SAVARR(game.odloc,NDWARVES);
	SAVARR(game.place,NOBJECTS);
	SAVARR(game.prop,NOBJECTS);
	SAVWRD(kk,K);
	if(K != 0) goto L8318;
	K=NUL;
	game.zzword=RNDVOC(3,game.zzword);
	if(kk > 0) return(8);
	RSPEAK(266);
	exit(0);

/*  Resume.  Read a suspended game back from a file. */

L8310:	kk=1;
	if(game.loc == 1 && game.abbrev[1] == 1) goto L8305;
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

L8320:	if(game.prop[RUG] != 2)SPK=224;
	if(!HERE(RUG))SPK=225;
	if(SPK/2 == 112) return(2011);
	obj=RUG;

L9320:	if(obj != RUG) return(2011);
	SPK=223;
	if(game.prop[RUG] != 2) return(2011);
	game.oldlc2=game.oldloc;
	game.oldloc=game.loc;
	game.newloc=game.place[RUG]+game.fixed[RUG]-game.loc;
	SPK=226;
	if(game.prop[SAPPH] >= 0)SPK=227;
	RSPEAK(SPK);
	 return(2);

L8330: return listen();

/*  Z'ZZZ (word gets recomputed at startup; different each game). */

L8340:	if(!AT(RESER) && game.loc != game.fixed[RESER]-1) return(2011);
	PSPEAK(RESER,game.prop[RESER]+1);
	game.prop[RESER]=1-game.prop[RESER];
	if(AT(RESER)) return(2012);
	game.oldlc2=game.loc;
	game.newloc=0;
	RSPEAK(241);
	 return(2);

}
