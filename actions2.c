#include "advent.h"
#include "funcs.h"

/*  Carry an object.  Special cases for bird and cage (if bird in cage, can't
 *  take one without the other).  Liquids also special, since they depend on
 *  status of bottle.  Also various side effects, etc. */

int carry(long obj)
{
    if (TOTING(obj)) return(2011);
    SPK=25;
    if (obj == PLANT && game.prop[PLANT] <= 0)SPK=115;
    if (obj == BEAR && game.prop[BEAR] == 1)SPK=169;
    if (obj == CHAIN && game.prop[BEAR] != 0)SPK=170;
    if (obj == URN)SPK=215;
    if (obj == CAVITY)SPK=217;
    if (obj == BLOOD)SPK=239;
    if (obj == RUG && game.prop[RUG] == 2)SPK=222;
    if (obj == SIGN)SPK=196;
    if (obj == MESSAG) {
	SPK=190;
	DSTROY(MESSAG);
    }
    if (game.fixed[obj] != 0)
	return(2011);
    if (obj == WATER || obj == OIL) {
	if (!HERE(BOTTLE) || LIQ(0) != obj) {
	    if (TOTING(BOTTLE) && game.prop[BOTTLE] == 1)
		return(fill(BOTTLE));
	    if (game.prop[BOTTLE] != 1)SPK=105;
	    if (!TOTING(BOTTLE))SPK=104;
	    return(2011);
	}
	obj = BOTTLE;
    }

    SPK=92;
    if (game.holdng >= INVLIMIT)
	return(2011);
    if (obj == BIRD && game.prop[BIRD] != 1 && -1-game.prop[BIRD] != 1) {
	if (game.prop[BIRD] == 2) {
	    SPK=238;
	    DSTROY(BIRD);
	    return(2011);
	}
	if (!TOTING(CAGE))SPK=27;
	if (TOTING(ROD))SPK=26;
	if (SPK/2 == 13) return(2011);
	game.prop[BIRD]=1;
    }
    if ((obj==BIRD || obj==CAGE) && (game.prop[BIRD]==1 || -1-game.prop[BIRD]==1))
	CARRY(BIRD+CAGE-obj,game.loc);
    CARRY(obj,game.loc);
    if (obj == BOTTLE && LIQ(0) != 0)
	game.place[LIQ(0)] = -1;
    if (!GSTONE(obj) || game.prop[obj] == 0)
	return(2009);
    game.prop[obj]=0;
    game.prop[CAVITY]=1;
    return(2009);
}

/*  Discard object.  "Throw" also comes here for most objects.  Special cases for
 *  bird (might attack snake or dragon) and cage (might contain bird) and vase.
 *  Drop coins at vending machine for extra batteries. */

int discard(long obj, bool just_do_it) {
	if (just_do_it) goto L9021;
	if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))obj=ROD2;
	if (!TOTING(obj)) return(2011);
	if (obj != BIRD || !HERE(SNAKE)) goto L9023;
	RSPEAK(30);
	if (game.closed) return(19000);
	DSTROY(SNAKE);
	/* Set game.prop for use by travel options */
	game.prop[SNAKE]=1;
L9021:	K=LIQ(0);
	if (K == obj)obj=BOTTLE;
	if (obj == BOTTLE && K != 0)game.place[K]=0;
	if (obj == CAGE && game.prop[BIRD] == 1)DROP(BIRD,game.loc);
	DROP(obj,game.loc);
	if (obj != BIRD) return(2012);
	game.prop[BIRD]=0;
	if (FOREST(game.loc))game.prop[BIRD]=2;
	 return(2012);

L9023:	if (!(GSTONE(obj) && AT(CAVITY) && game.prop[CAVITY] != 0)) goto L9024;
	RSPEAK(218);
	game.prop[obj]=1;
	game.prop[CAVITY]=0;
	if (!HERE(RUG) || !((obj == EMRALD && game.prop[RUG] != 2) || (obj == RUBY &&
		game.prop[RUG] == 2))) goto L9021;
	SPK=219;
	if (TOTING(RUG))SPK=220;
	if (obj == RUBY)SPK=221;
	RSPEAK(SPK);
	if (SPK == 220) goto L9021;
	K=2-game.prop[RUG];
	game.prop[RUG]=K;
	if (K == 2)K=PLAC[SAPPH];
	MOVE(RUG+NOBJECTS,K);
	 goto L9021;

L9024:	if (obj != COINS || !HERE(VEND)) goto L9025;
	DSTROY(COINS);
	DROP(BATTER,game.loc);
	PSPEAK(BATTER,0);
	 return(2012);

L9025:	if (obj != BIRD || !AT(DRAGON) || game.prop[DRAGON] != 0) goto L9026;
	RSPEAK(154);
	DSTROY(BIRD);
	game.prop[BIRD]=0;
	 return(2012);

L9026:	if (obj != BEAR || !AT(TROLL)) goto L9027;
	RSPEAK(163);
	MOVE(TROLL,0);
	MOVE(TROLL+NOBJECTS,0);
	MOVE(TROLL2,PLAC[TROLL]);
	MOVE(TROLL2+NOBJECTS,FIXD[TROLL]);
	JUGGLE(CHASM);
	game.prop[TROLL]=2;
	 goto L9021;

L9027:	if (obj == VASE && game.loc != PLAC[PILLOW]) goto L9028;
	RSPEAK(54);
	 goto L9021;

L9028:	game.prop[VASE]=2;
	if (AT(PILLOW))game.prop[VASE]=0;
	PSPEAK(VASE,game.prop[VASE]+1);
	if (game.prop[VASE] != 0)game.fixed[VASE]= -1;
	 goto L9021;
}

/*  Attack.  Assume target if unambiguous.  "Throw" also links here.  Attackable
 *  objects fall into two categories: enemies (snake, dwarf, etc.)  and others
 *  (bird, clam, machine).  Ambiguous if 2 enemies, or no enemies but 2 others. */

int attack(FILE *input, long obj, long verb) {
	I=ATDWRF(game.loc);
	if (obj != 0) goto L9124;
	if (I > 0)obj=DWARF;
	if (HERE(SNAKE))obj=obj*NOBJECTS+SNAKE;
	if (AT(DRAGON) && game.prop[DRAGON] == 0)obj=obj*NOBJECTS+DRAGON;
	if (AT(TROLL))obj=obj*NOBJECTS+TROLL;
	if (AT(OGRE))obj=obj*NOBJECTS+OGRE;
	if (HERE(BEAR) && game.prop[BEAR] == 0)obj=obj*NOBJECTS+BEAR;
	if (obj > NOBJECTS) return(8000);
	if (obj != 0) goto L9124;
	/* Can't attack bird or machine by throwing axe. */
	if (HERE(BIRD) && verb != THROW)obj=BIRD;
	if (HERE(VEND) && verb != THROW)obj=obj*NOBJECTS+VEND;
	/* Clam and oyster both treated as clam for intransitive case;
	 * no harm done. */
	if (HERE(CLAM) || HERE(OYSTER))obj=NOBJECTS*obj+CLAM;
	if (obj > NOBJECTS) return(8000);
L9124:	if (obj == BIRD) {
		SPK=137;
		if (game.closed) return(2011);
		DSTROY(BIRD);
		game.prop[BIRD]=0;
		SPK=45;
	}
L9125:	if (obj == VEND) {
	    PSPEAK(VEND,game.prop[VEND]+2);
	    game.prop[VEND]=3-game.prop[VEND];
	    return(2012);
	}

	if (obj == 0)SPK=44;
	if (obj == CLAM || obj == OYSTER)SPK=150;
	if (obj == SNAKE)SPK=46;
	if (obj == DWARF)SPK=49;
	if (obj == DWARF && game.closed) return(19000);
	if (obj == DRAGON)SPK=167;
	if (obj == TROLL)SPK=157;
	if (obj == OGRE)SPK=203;
	if (obj == OGRE && I > 0) goto L9128;
	if (obj == BEAR)SPK=165+(game.prop[BEAR]+1)/2;
	if (obj != DRAGON || game.prop[DRAGON] != 0) return(2011);
	/*  Fun stuff for dragon.  If he insists on attacking it, win!
	 *  Set game.prop to dead, move dragon to central loc (still
	 *  fixed), move rug there (not fixed), and move him there,
	 *  too.  Then do a null motion to get new description. */
	RSPEAK(49);
	GETIN(input,WD1,WD1X,WD2,WD2X);
	if (WD1 != MAKEWD(25) && WD1 != MAKEWD(250519)) return(2607);
	PSPEAK(DRAGON,3);
	game.prop[DRAGON]=1;
	game.prop[RUG]=0;
	K=(PLAC[DRAGON]+FIXD[DRAGON])/2;
	MOVE(DRAGON+NOBJECTS,-1);
	MOVE(RUG+NOBJECTS,0);
	MOVE(DRAGON,K);
	MOVE(RUG,K);
	DROP(BLOOD,K);
	for (obj=1; obj<=NOBJECTS; obj++) {
	if (game.place[obj] == PLAC[DRAGON] || game.place[obj] == FIXD[DRAGON])MOVE(obj,K);
	/*etc*/ ;
	} /* end loop */
	game.loc=K;
	K=NUL;
	 return(8);

L9128:	RSPEAK(SPK);
	RSPEAK(6);
	DSTROY(OGRE);
	K=0;
	for (I=1; I < PIRATE; I++) {
		if (game.dloc[I] == game.loc) {
			K=K+1;
			game.dloc[I]=61;
			game.dseen[I]=false;
		}
	}
	SPK=SPK+1+1/K;
	return(2011);
}

/*  Throw.  Same as discard unless axe.  Then same as attack except ignore bird,
 *  and if dwarf is present then one might be killed.  (Only way to do so!)
 *  Axe also special for dragon, bear, and troll.  Treasures special for troll. */

int throw(FILE *cmdin, long obj, long verb) {
	if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))obj=ROD2;
	if (!TOTING(obj)) return(2011);
	if (obj >= 50 && obj <= MAXTRS && AT(TROLL)) goto L9178;
	if (obj == FOOD && HERE(BEAR)) goto L9177;
	if (obj != AXE) return(discard(obj, false));
	I=ATDWRF(game.loc);
	if (I > 0) goto L9172;
	SPK=152;
	if (AT(DRAGON) && game.prop[DRAGON] == 0) goto L9175;
	SPK=158;
	if (AT(TROLL)) goto L9175;
	SPK=203;
	if (AT(OGRE)) goto L9175;
	if (HERE(BEAR) && game.prop[BEAR] == 0) goto L9176;
	obj=0;
	return(attack(cmdin, obj, verb));

L9172:	SPK=48;
	if (randrange(NDWARVES+1) < game.dflag) goto L9175;
	game.dseen[I]=false;
	game.dloc[I]=0;
	SPK=47;
	game.dkill=game.dkill+1;
	if (game.dkill == 1)SPK=149;
L9175:	RSPEAK(SPK);
	DROP(AXE,game.loc);
	K=NUL;
	 return(8);

/*  This'll teach him to throw the axe at the bear! */
L9176:	SPK=164;
	DROP(AXE,game.loc);
	game.fixed[AXE]= -1;
	game.prop[AXE]=1;
	JUGGLE(BEAR);
	 return(2011);

/*  But throwing food is another story. */
L9177:	obj=BEAR;
	return(feed(obj));

L9178:	SPK=159;
/*  Snarf a treasure for the troll. */
	DROP(obj,0);
	MOVE(TROLL,0);
	MOVE(TROLL+NOBJECTS,0);
	DROP(TROLL2,PLAC[TROLL]);
	DROP(TROLL2+NOBJECTS,FIXD[TROLL]);
	JUGGLE(CHASM);
	 return(2011);
}

int feed(long obj)
/*  Feed.  If bird, no seed.  Snake, dragon, troll: quip.  If dwarf, make him
 *  mad.  Bear, special. */
{
    if (obj == BIRD) {
	SPK=100;
	return(2011);
    }

    if (!(obj != SNAKE && obj != DRAGON && obj != TROLL)) {
	SPK=102;
	if (obj == DRAGON && game.prop[DRAGON] != 0)SPK=110;
	if (obj == TROLL)SPK=182;
	if (obj != SNAKE || game.closed || !HERE(BIRD))
	    return(2011);
	SPK=101;
	DSTROY(BIRD);
	game.prop[BIRD]=0;
	return(2011);
    }

    if (obj == DWARF) {
	if (!HERE(FOOD))
	    return(2011);
	SPK=103;
	game.dflag=game.dflag+2;
	return(2011);
    }

    if (obj == BEAR) {
	if (game.prop[BEAR] == 0)SPK=102;
	if (game.prop[BEAR] == 3)SPK=110;
	if (!HERE(FOOD))
	    return(2011);
	DSTROY(FOOD);
	game.prop[BEAR]=1;
	game.fixed[AXE]=0;
	game.prop[AXE]=0;
	SPK=168;
	return(2011);
    }

    if (obj == OGRE) {
	if (HERE(FOOD))
	    SPK=202;
	return(2011);
    }

    SPK=14;
    return(2011);
}

int fill(long obj)
/*  Fill.  Bottle or urn must be empty, and liquid available.  (Vase
 *  is nasty.) */
{
    if (obj == VASE) {
	SPK=29;
	if (LIQLOC(game.loc) == 0)SPK=144;
	if (LIQLOC(game.loc) == 0 || !TOTING(VASE))
	    return(2011);
	RSPEAK(145);
	game.prop[VASE]=2;
	game.fixed[VASE]= -1;
	return(discard(obj, true));
    }

    if (obj == URN){
	SPK=213;
	if (game.prop[URN] != 0) return(2011);
	SPK=144;
	K=LIQ(0);
	if (K == 0 || !HERE(BOTTLE)) return(2011);
	game.place[K]=0;
	game.prop[BOTTLE]=1;
	if (K == OIL)game.prop[URN]=1;
	SPK=211+game.prop[URN];
	return(2011);
    }

    if (obj != 0 && obj != BOTTLE)
	return(2011);
    if (obj == 0 && !HERE(BOTTLE))
	return(8000);
    SPK=107;
    if (LIQLOC(game.loc) == 0)
	SPK=106;
    if (HERE(URN) && game.prop[URN] != 0)
	SPK=214;
    if (LIQ(0) != 0)
	SPK=105;
    if (SPK != 107)
	return(2011);
    game.prop[BOTTLE]=MOD(COND[game.loc],4)/2*2;
    K=LIQ(0);
    if (TOTING(BOTTLE))
	game.place[K]= -1;
    if (K == OIL)
	SPK=108;
    return(2011);
}
