#include <stdlib.h>
#include <stdbool.h>
#include "advent.h"
#include "database.h"

/*
 * Action handlers.  Eventually we'll do lookup through a method table
 * that calls these.  Absolutely nothing like the original FORTRAN.
 */

static int fill(long);

static int attack(FILE *input, long verb, long obj)
/*  Attack.  Assume target if unambiguous.  "Throw" also links here.
 *  Attackable objects fall into two categories: enemies (snake,
 *  dwarf, etc.)  and others (bird, clam, machine).  Ambiguous if 2
 *  enemies, or no enemies but 2 others. */
{
    int i =ATDWRF(game.loc);
    if (obj == 0) {
	if (i > 0)
	    obj=DWARF;
	if (HERE(SNAKE))obj=obj*NOBJECTS+SNAKE;
	if (AT(DRAGON) && game.prop[DRAGON] == 0)obj=obj*NOBJECTS+DRAGON;
	if (AT(TROLL))obj=obj*NOBJECTS+TROLL;
	if (AT(OGRE))obj=obj*NOBJECTS+OGRE;
	if (HERE(BEAR) && game.prop[BEAR] == 0)obj=obj*NOBJECTS+BEAR;
	if (obj > NOBJECTS) return(8000);
	if (obj == 0) {
	    /* Can't attack bird or machine by throwing axe. */
	    if (HERE(BIRD) && verb != THROW)obj=BIRD;
	    if (HERE(VEND) && verb != THROW)obj=obj*NOBJECTS+VEND;
	    /* Clam and oyster both treated as clam for intransitive case;
	     * no harm done. */
	    if (HERE(CLAM) || HERE(OYSTER))obj=NOBJECTS*obj+CLAM;
	    if (obj > NOBJECTS) return(8000);
	}
    }
    if (obj == BIRD) {
	SPK=137;
	if (game.closed) return(2011);
	DSTROY(BIRD);
	game.prop[BIRD]=0;
	SPK=45;
    }
    if (obj == VEND) {
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
    if (obj == OGRE && i > 0) {
	RSPEAK(SPK);
	RSPEAK(6);
	DSTROY(OGRE);
	int k=0;
	for (i=1; i < PIRATE; i++) {
	    if (game.dloc[i] == game.loc) {
		++k;
		game.dloc[i]=61;
		game.dseen[i]=false;
	    }
	}
	SPK=SPK+1+1/k;
	return(2011);
    }
    if (obj == BEAR)SPK=165+(game.prop[BEAR]+1)/2;
    if (obj != DRAGON || game.prop[DRAGON] != 0) return(2011);
    /*  Fun stuff for dragon.  If he insists on attacking it, win!
     *  Set game.prop to dead, move dragon to central loc (still
     *  fixed), move rug there (not fixed), and move him there,
     *  too.  Then do a null motion to get new description. */
    RSPEAK(49);
    GETIN(input,&WD1,&WD1X,&WD2,&WD2X);
    if (WD1 != MAKEWD(25) && WD1 != MAKEWD(250519))
	return(2607);
    PSPEAK(DRAGON,3);
    game.prop[DRAGON]=1;
    game.prop[RUG]=0;
    int k=(PLAC[DRAGON]+FIXD[DRAGON])/2;
    MOVE(DRAGON+NOBJECTS,-1);
    MOVE(RUG+NOBJECTS,0);
    MOVE(DRAGON,k);
    MOVE(RUG,k);
    DROP(BLOOD,k);
    for (obj=1; obj<=NOBJECTS; obj++) {
	if (game.place[obj] == PLAC[DRAGON] || game.place[obj] == FIXD[DRAGON])
	    MOVE(obj,k);
    }
    game.loc=k;
    K=NUL;	/* FIXME: error if removed */
    return(8);
}

static int bigwords(long foo)
/*  FEE FIE FOE FOO (AND FUM).  Advance to next state if given in proper order.
 *  Look up WD1 in section 3 of vocab to determine which word we've got.  Last
 *  word zips the eggs back to the giant room (unless already there). */
{
    int k=VOCAB(foo,3);
    SPK=42;
    if (game.foobar != 1-k) {
	if (game.foobar != 0)SPK=151;
	return(2011);
    } else {
	game.foobar=k;
	if (k != 4) return(2009);
	game.foobar=0;
	if (game.place[EGGS] == PLAC[EGGS] || (TOTING(EGGS) && game.loc == PLAC[EGGS])) 
	    return(2011);
	/*  Bring back troll if we steal the eggs back from him before
	 *  crossing. */
	if (game.place[EGGS]==0 && game.place[TROLL]==0 && game.prop[TROLL]==0)
	    game.prop[TROLL]=1;
	k=2;
	if (HERE(EGGS))k=1;
	if (game.loc == PLAC[EGGS])k=0;
	MOVE(EGGS,PLAC[EGGS]);
	PSPEAK(EGGS,k);
	return(2012);
    }
}

static int bivalve(token_t verb, token_t obj)
/* Clam/oyster actions */
{
    int k=0;
    if (obj == OYSTER)k=1;
    SPK=124+k;
    if (TOTING(obj))SPK=120+k;
    if (!TOTING(TRIDNT))SPK=122+k;
    if (verb == LOCK)SPK=61;
    if (SPK != 124)
	return(2011);
    DSTROY(CLAM);
    DROP(OYSTER,game.loc);
    DROP(PEARL,105);
    return(2011);
}

static int blast(void)
/*  Blast.  No effect unless you've got dynamite, which is a neat trick! */
{
    if (game.prop[ROD2] < 0 || !game.closed) return(2011);
    game.bonus=133;
    if (game.loc == 115)game.bonus=134;
    if (HERE(ROD2))game.bonus=135;
    RSPEAK(game.bonus);
    score(0);
}

static int vbreak(token_t obj)
/*  Break.  Only works for mirror in repository and, of course, the vase. */
{
    if (obj == MIRROR)SPK=148;
    if (obj == VASE && game.prop[VASE] == 0) {
	SPK=198;
	if (TOTING(VASE))DROP(VASE,game.loc);
	game.prop[VASE]=2;
	game.fixed[VASE]= -1;
	return(2011);
    } else {
	if (obj != MIRROR || !game.closed) return(2011);
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

static int carry(long obj)
/*  Carry an object.  Special cases for bird and cage (if bird in cage, can't
 *  take one without the other).  Liquids also special, since they depend on
 *  status of bottle.  Also various side effects, etc. */
{
    if (obj == INTRANSITIVE) {
	/*  Carry, no object given yet.  OK if only one object present. */
	if(game.atloc[game.loc] == 0 ||
	   game.link[game.atloc[game.loc]] != 0 ||
	   ATDWRF(game.loc) > 0)
	    return(8000);
	obj=game.atloc[game.loc];
    }

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

static int chain(token_t verb)
/* Do something to the bear's chain */
{
    if (verb != LOCK) {
	SPK=171;
	if (game.prop[BEAR] == 0)SPK=41;
	if (game.prop[CHAIN] == 0)SPK=37;
	if (SPK != 171) return(2011);
	game.prop[CHAIN]=0;
	game.fixed[CHAIN]=0;
	if (game.prop[BEAR] != 3)game.prop[BEAR]=2;
	game.fixed[BEAR]=2-game.prop[BEAR];
	return(2011);
    } else {
	SPK=172;
	if (game.prop[CHAIN] != 0)SPK=34;
	if (game.loc != PLAC[CHAIN])SPK=173;
	if (SPK != 172) return(2011);
	game.prop[CHAIN]=2;
	if (TOTING(CHAIN))DROP(CHAIN,game.loc);
	game.fixed[CHAIN]= -1;
	return(2011);
    }
}

static int discard(long obj, bool just_do_it)
/*  Discard object.  "Throw" also comes here for most objects.  Special cases for
 *  bird (might attack snake or dragon) and cage (might contain bird) and vase.
 *  Drop coins at vending machine for extra batteries. */
{
    if (!just_do_it) {
        if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))obj=ROD2;
        if (!TOTING(obj)) return(2011);
        if (obj == BIRD && HERE(SNAKE)) {
            RSPEAK(30);
            if (game.closed) return(19000);
            DSTROY(SNAKE);
            /* Set game.prop for use by travel options */
            game.prop[SNAKE]=1;

        } else if ((GSTONE(obj) && AT(CAVITY) && game.prop[CAVITY] != 0)) {
            RSPEAK(218);
            game.prop[obj]=1;
            game.prop[CAVITY]=0;
            if (HERE(RUG) && ((obj == EMRALD && game.prop[RUG] != 2) || (obj == RUBY &&
									 game.prop[RUG] == 2))) {
                SPK=219;
                if (TOTING(RUG))SPK=220;
                if (obj == RUBY)SPK=221;
                RSPEAK(SPK);
                if (SPK != 220) {
                    K=2-game.prop[RUG];
                    game.prop[RUG]=K;
                    if (K == 2)K=PLAC[SAPPH];
                    MOVE(RUG+NOBJECTS,K);
                }
            }
        } else if (obj == COINS && HERE(VEND)) {
            DSTROY(COINS);
            DROP(BATTER,game.loc);
            PSPEAK(BATTER,0);
            return(2012);
        } else if (obj == BIRD && AT(DRAGON) && game.prop[DRAGON] == 0) {
            RSPEAK(154);
            DSTROY(BIRD);
            game.prop[BIRD]=0;
            return(2012);
        } else if (obj == BEAR && AT(TROLL)) {
            RSPEAK(163);
            MOVE(TROLL,0);
            MOVE(TROLL+NOBJECTS,0);
            MOVE(TROLL2,PLAC[TROLL]);
            MOVE(TROLL2+NOBJECTS,FIXD[TROLL]);
            JUGGLE(CHASM);
            game.prop[TROLL]=2;
        } else if (obj != VASE || game.loc == PLAC[PILLOW]) {
            RSPEAK(54);
        } else {
            game.prop[VASE]=2;
            if (AT(PILLOW))game.prop[VASE]=0;
            PSPEAK(VASE,game.prop[VASE]+1);
            if (game.prop[VASE] != 0)game.fixed[VASE]= -1;
        }
    }
    K=LIQ(0);
    if (K == obj)obj=BOTTLE;
    if (obj == BOTTLE && K != 0)game.place[K]=0;
    if (obj == CAGE && game.prop[BIRD] == 1)DROP(BIRD,game.loc);
    DROP(obj,game.loc);
    if (obj != BIRD) return(2012);
    game.prop[BIRD]=0;
    if (FOREST(game.loc))game.prop[BIRD]=2;
    return(2012);
}

static int drink(token_t obj)
/*  Drink.  If no object, assume water and look for it here.  If water is in
 *  the bottle, drink that, else must be at a water loc, so drink stream. */
{
    if (obj == 0 && LIQLOC(game.loc) != WATER && (LIQ(0) != WATER || !HERE(BOTTLE)))
	return(8000);
    if (obj != BLOOD) {
	if (obj != 0 && obj != WATER)SPK=110;
	if (SPK == 110 || LIQ(0) != WATER || !HERE(BOTTLE)) return(2011);
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

static int eat(token_t obj)
/*  Eat.  Intransitive: assume food if present, else ask what.  Transitive: food
 *  ok, some things lose appetite, rest are ridiculous. */
{
    if (obj == INTRANSITIVE) {
	if (!HERE(FOOD))
	    return(8000);
	DSTROY(FOOD);
	SPK=72;
	return(2011);
    } else {
	if (obj == FOOD) {
	    DSTROY(FOOD);
	    SPK=72;
	    return(2011);
	}
	if (obj == BIRD || obj == SNAKE || obj == CLAM || obj == OYSTER || obj ==
	   DWARF || obj == DRAGON || obj == TROLL || obj == BEAR || obj ==
	   OGRE)SPK=71;
	return(2011);
    }
}

static int extinguish(int obj)
/* Extinguish.  Lamp, urn, dragon/volcano (nice try). */
{
    if (obj == INTRANSITIVE) {
	if (HERE(LAMP) && game.prop[LAMP] == 1)obj=LAMP;
	if (HERE(URN) && game.prop[URN] == 2)obj=obj*NOBJECTS+URN;
	if (obj == 0 || obj > NOBJECTS) return(8000);
    }

    if (obj == URN) {
	game.prop[URN]=game.prop[URN]/2;
	SPK=210;
	return(2011);
    }
    if (obj == LAMP) {
	game.prop[LAMP]=0;
	RSPEAK(40);
	if (DARK(0))
	    RSPEAK(16);
	return(2012);
    }
    if (obj == DRAGON || obj == VOLCAN)
	SPK=146;
    return(2011);
}

static int feed(long obj)
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
    int k;
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
	k=LIQ(0);
	if (k == 0 || !HERE(BOTTLE)) return(2011);
	game.place[k]=0;
	game.prop[BOTTLE]=1;
	if (k == OIL)game.prop[URN]=1;
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
    k=LIQ(0);
    if (TOTING(BOTTLE))
	game.place[k]= -1;
    if (k == OIL)
	SPK=108;
    return(2011);
}

static int find(token_t obj)
/* Find.  Might be carrying it, or it might be here.  Else give caveat. */
{
    if (AT(obj) ||
       (LIQ(0) == obj && AT(BOTTLE)) ||
       obj == LIQLOC(game.loc) ||
       (obj == DWARF && ATDWRF(game.loc) > 0))
	SPK=94;
    if (game.closed)SPK=138;
    if (TOTING(obj))SPK=24;
    return(2011);
}

static int fly(token_t obj)
/* Fly.  Snide remarks unless hovering rug is here. */
{
    if (obj == INTRANSITIVE) {
	if (game.prop[RUG] != 2)SPK=224;
	if (!HERE(RUG))SPK=225;
	if (SPK/2 == 112) return(2011);
	obj=RUG;
    }

    if (obj != RUG) return(2011);
    SPK=223;
    if (game.prop[RUG] != 2) return(2011);
    game.oldlc2=game.oldloc;
    game.oldloc=game.loc;
    game.newloc=game.place[RUG]+game.fixed[RUG]-game.loc;
    SPK=226;
    if (game.prop[SAPPH] >= 0)SPK=227;
    RSPEAK(SPK);
    return(2);
}
    
static int inven(token_t obj)
/* Inventory. If object, treat same as find.  Else report on current burden. */
{
    int i;
    SPK=98;
    for (i=1; i<=NOBJECTS; i++) {
	if (i == BEAR || !TOTING(i))
	    continue;
	if (SPK == 98)RSPEAK(99);
	game.blklin=false;
	PSPEAK(i,-1);
	game.blklin=true;
	SPK=0;
    }
    if (TOTING(BEAR))
	SPK=141;
    return(2011);
}

int light(token_t obj)
/*  Light.  Applicable only to lamp and urn. */
{
    if (obj == INTRANSITIVE) {
	if (HERE(LAMP) && game.prop[LAMP] == 0 && game.limit >= 0)obj=LAMP;
	if (HERE(URN) && game.prop[URN] == 1)obj=obj*NOBJECTS+URN;
	if (obj == 0 || obj > NOBJECTS) return(8000);
    }

    if (obj == URN) {
	SPK=38;
	if (game.prop[URN] == 0)
	    return(2011);
	SPK=209;
	game.prop[URN]=2;
	return(2011);
    } else {
	if (obj != LAMP) return(2011);
	SPK=184;
	if (game.limit < 0) return(2011);
	game.prop[LAMP]=1;
	RSPEAK(39);
	if (game.wzdark) return(2000);
	return(2012);
    }	 
}

static int listen(void)
/*  Listen.  Intransitive only.  Print stuff based on objsnd/locsnd. */
{
    int i, k;
    SPK=228;
    k=LOCSND[game.loc];
    if (k != 0) {
	RSPEAK(labs(k));
	if (k < 0) return(2012);
	SPK=0;
    }
    SETPRM(1,game.zzword,0);
    for (i=1; i<=NOBJECTS; i++) {
	if (!HERE(i) || OBJSND[i] == 0 || game.prop[i] < 0)
	    continue;
	PSPEAK(i,OBJSND[i]+game.prop[i]);
	SPK=0;
	if (i == BIRD && OBJSND[i]+game.prop[i] == 8)
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
	if (HERE(CLAM))obj=CLAM;
	if (HERE(OYSTER))obj=OYSTER;
	if (AT(DOOR))obj=DOOR;
	if (AT(GRATE))obj=GRATE;
	if (obj != 0 && HERE(CHAIN)) return(8000);
	if (HERE(CHAIN))obj=CHAIN;
	if (obj == 0) return(2011);
    }
	
    /*  Lock, unlock object.  Special stuff for opening clam/oyster
     *  and for chain. */
    if (obj == CLAM || obj == OYSTER)
	return bivalve(verb, obj);
    if (obj == DOOR)SPK=111;
    if (obj == DOOR && game.prop[DOOR] == 1)SPK=54;
    if (obj == CAGE)SPK=32;
    if (obj == KEYS)SPK=55;
    if (obj == GRATE || obj == CHAIN)SPK=31;
    if (SPK != 31 || !HERE(KEYS)) return(2011);
    if (obj == CHAIN)
	return chain(verb);
    if (game.closng) {
	SPK=130;
	if (!game.panic)game.clock2=15;
	game.panic=true;
	return(2011);
    }
    SPK=34+game.prop[GRATE];
    game.prop[GRATE]=1;
    if (verb == LOCK)game.prop[GRATE]=0;
    SPK=SPK+2*game.prop[GRATE];
    return(2011);
}

static int pour(token_t obj)
/*  Pour.  If no object, or object is bottle, assume contents of bottle.
 *  special tests for pouring water or oil on plant or rusty door. */
{
    if (obj == BOTTLE || obj == 0)obj=LIQ(0);
    if (obj == 0) return(8000);
    if (!TOTING(obj)) return(2011);
    SPK=78;
    if (obj != OIL && obj != WATER) return(2011);
    if (HERE(URN) && game.prop[URN] == 0)
	return fill(URN);
    game.prop[BOTTLE]=1;
    game.place[obj]=0;
    SPK=77;
    if (!(AT(PLANT) || AT(DOOR)))
	return(2011);
    if (!AT(DOOR)) {
	SPK=112;
	if (obj != WATER) return(2011);
	PSPEAK(PLANT,game.prop[PLANT]+3);
	game.prop[PLANT]=MOD(game.prop[PLANT]+1,3);
	game.prop[PLANT2]=game.prop[PLANT];
	K=NUL;
	return(8);
    } else {
	game.prop[DOOR]=0;
	if (obj == OIL)game.prop[DOOR]=1;
	SPK=113+game.prop[DOOR];
	return(2011);
    }
}

static int quit(FILE *input)
/*  Quit.  Intransitive only.  Verify intent and exit if that's what he wants. */
{
    if (YES(input,22,54,54))
	score(1);
    return(2012);
}

static int read(FILE *input, token_t obj)
/*  Read.  Print stuff based on objtxt.  Oyster (?) is special case. */
{
    int i;
    if (obj == INTRANSITIVE) {
	obj = 0;
	for (i=1; i<=NOBJECTS; i++) {
	    if (HERE(i) && OBJTXT[i] != 0 && game.prop[i] >= 0)
		obj = obj * NOBJECTS + i;
	}
	if (obj > NOBJECTS || obj == 0 || DARK(0)) return(8000);
    }
	
    if (DARK(0)) {
	SETPRM(1,WD1,WD1X);
	RSPEAK(256);
	return(2012);
    }
    if (OBJTXT[obj] == 0 || game.prop[obj] < 0)
	return(2011);
    if (obj == OYSTER && !game.clshnt) {
	game.clshnt=YES(input,192,193,54);
	return(2012);
    }
    PSPEAK(obj,OBJTXT[obj]+game.prop[obj]);
    return(2012);
}

static int reservoir(void)
/*  Z'ZZZ (word gets recomputed at startup; different each game). */
{
    if (!AT(RESER) && game.loc != game.fixed[RESER]-1) return(2011);
    PSPEAK(RESER,game.prop[RESER]+1);
    game.prop[RESER]=1-game.prop[RESER];
    if (AT(RESER)) return(2012);
    game.oldlc2=game.loc;
    game.newloc=0;
    RSPEAK(241);
    return(2);
}

static int rub(token_t obj)
/* Rub.  Yields various snide remarks except for lit urn. */
{
    if (obj != LAMP)SPK=76;
    if (obj != URN || game.prop[URN] != 2) return(2011);
    DSTROY(URN);
    DROP(AMBER,game.loc);
    game.prop[AMBER]=1;
    --game.tally;
    DROP(CAVITY,game.loc);
    SPK=216;
    return(2011);
}

static int say(void)
/* SAY.  Echo WD2 (or WD1 if no WD2 (SAY WHAT?, etc.).)  Magic words override. */
{
    /* FIXME: ugly use of globals */
    SETPRM(1,WD2,WD2X); if (WD2 <= 0)SETPRM(1,WD1,WD1X);
    if (WD2 > 0)WD1=WD2;
    int wd=VOCAB(WD1,-1);
    if (wd == 62 || wd == 65 || wd == 71 || wd == 2025 || wd == 2034) {
	WD2=0;
	return(2630);
    }
    RSPEAK(258);
    return(2012);

}

static int throw_support(long spk)
{
    RSPEAK(spk);
    DROP(AXE,game.loc);
    K=NUL;
    return(8);
}

static int throw(FILE *cmdin, long verb, long obj)
/*  Throw.  Same as discard unless axe.  Then same as attack except
 *  ignore bird, and if dwarf is present then one might be killed.
 *  (Only way to do so!)  Axe also special for dragon, bear, and
 *  troll.  Treasures special for troll. */
{
    if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))obj=ROD2;
    if (!TOTING(obj))
	return(2011);
    if (obj >= 50 && obj <= MAXTRS && AT(TROLL)) {
        SPK=159;
        /*  Snarf a treasure for the troll. */
        DROP(obj,0);
        MOVE(TROLL,0);
        MOVE(TROLL+NOBJECTS,0);
        DROP(TROLL2,PLAC[TROLL]);
        DROP(TROLL2+NOBJECTS,FIXD[TROLL]);
        JUGGLE(CHASM);
        return(2011);
    }
    if (obj == FOOD && HERE(BEAR)) {
    /* But throwing food is another story. */
        obj=BEAR;
        return(feed(obj));
    }
    if (obj != AXE)
	return(discard(obj, false));
    int i=ATDWRF(game.loc);
    if (i <= 0) {
        if (AT(DRAGON) && game.prop[DRAGON] == 0) {
            SPK=152;
            return throw_support(SPK);
        }
        if (AT(TROLL)) {
            SPK=158;
            return throw_support(SPK);
        }
        if (AT(OGRE)) {
            SPK=203;
            return throw_support(SPK);
        }
        if (HERE(BEAR) && game.prop[BEAR] == 0) {
            /* This'll teach him to throw the axe at the bear! */
            SPK=164;
            DROP(AXE,game.loc);
            game.fixed[AXE]= -1;
            game.prop[AXE]=1;
            JUGGLE(BEAR);
            return(2011);
        }
        return(attack(cmdin, verb, 0));
    }

    if (randrange(NDWARVES+1) < game.dflag) {
        SPK=48;
        return throw_support(SPK);
    }
    game.dseen[i]=false;
    game.dloc[i]=0;
    SPK=47;
    ++game.dkill;
    if (game.dkill == 1)SPK=149;

    return throw_support(SPK);
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
    if (obj != DWARF || !game.closed) return(2011);
    SPK=199;
    return(18999);
}

static int wave(token_t obj)
/* Wave.  No effect unless waving rod at fissure or at bird. */
{
    if ((!TOTING(obj)) && (obj != ROD || !TOTING(ROD2)))SPK=29;
    if (obj != ROD ||
       !TOTING(obj) ||
       (!HERE(BIRD) && (game.closng || !AT(FISSUR))))
	return(2011);
    if (HERE(BIRD))SPK=206+MOD(game.prop[BIRD],2);
    if (SPK == 206 && game.loc == game.place[STEPS] && game.prop[JADE] < 0) {
	DROP(JADE,game.loc);
	game.prop[JADE]=0;
	--game.tally;
	SPK=208;
	return(2011);
    } else {
	if (game.closed) return(18999);
	if (game.closng || !AT(FISSUR)) return(2011);
	if (HERE(BIRD))RSPEAK(SPK);
	game.prop[FISSUR]=1-game.prop[FISSUR];
	PSPEAK(FISSUR,2-game.prop[FISSUR]);
	return(2012);
    }
}

/* We're called with a number that says what label the caller wanted
 * to "goto", and we return a similar label number for the caller to
 * "goto".
 */

int action(FILE *input, enum speechpart part, long verb, long obj)
/*  Analyse a verb.  Remember what it was, go back for object if second word
 *  unless verb is "say", which snarfs arbitrary second word.
 */
{
    int kk;
    switch(part)
    {
	case intransitive:
	    SPK=ACTSPK[verb];
	    if (WD2 > 0 && verb != SAY) return(2800);
	    if (verb == SAY)obj=WD2;
	    if (obj == 0) {
		/*  Analyse an intransitive verb (ie, no object given yet). */
		switch (verb-1) {
		    case  0: /* CARRY */ return carry(INTRANSITIVE);
		    case  1: /* DROP  */ return(8000); 
		    case  2: /* SAY   */ return(8000); 
		    case  3: /* UNLOC */ return lock(verb, INTRANSITIVE);    
		    case  4: /* NOTHI */ return(2009); 
		    case  5: /* LOCK  */ return lock(verb, INTRANSITIVE);    
		    case  6: /* LIGHT */ return light(INTRANSITIVE);    
		    case  7: /* EXTIN */ return extinguish(INTRANSITIVE);    
		    case  8: /* WAVE  */ return(8000); 
		    case  9: /* CALM  */ return(8000); 
		    case 10: /* WALK  */ return(2011); 
		    case 11: /* ATTAC */ return attack(input, verb, obj);   
		    case 12: /* POUR  */ return pour(obj);   
		    case 13: /* EAT   */ return eat(INTRANSITIVE);   
		    case 14: /* DRINK */ return drink(obj);   
		    case 15: /* RUB   */ return(8000); 
		    case 16: /* TOSS  */ return(8000); 
		    case 17: /* QUIT  */ return quit(input);   
		    case 18: /* FIND  */ return(8000); 
		    case 19: /* INVEN */ return inven(obj);   
		    case 20: /* FEED  */ return(8000); 
		    case 21: /* FILL  */ return fill(obj);   
		    case 22: /* BLAST */ return blast();   
		    case 23: /* SCOR  */ return vscore();   
		    case 24: /* FOO   */ return bigwords(WD1);   
		    case 25: /* BRIEF */ return brief();   
		    case 26: /* READ  */ return read(input, INTRANSITIVE);   
		    case 27: /* BREAK */ return(8000); 
		    case 28: /* WAKE  */ return(8000); 
		    case 29: /* SUSP  */ return saveresume(input, false);   
		    case 30: /* RESU  */ return saveresume(input, true);   
		    case 31: /* FLY   */ return fly(INTRANSITIVE);   
		    case 32: /* LISTE */ return listen();   
		    case 33: /* ZZZZ  */ return reservoir();   
		}
		BUG(23);
	    }
	    /* FALLTHRU */
	case transitive:
	L4090:
	    /*  Analyse a transitive verb. */
	    switch (verb-1) {
		case  0: /* CARRY */ return carry(obj);    
		case  1: /* DROP  */ return discard(obj, false);    
		case  2: /* SAY   */ return say();    
		case  3: /* UNLOC */ return lock(verb, obj);    
		case  4: /* NOTHI */ return(2009); 
		case  5: /* LOCK  */ return lock(verb, obj);    
		case  6: /* LIGHT */ return light(obj);    
		case  7: /* EXTI  */ return extinguish(obj);    
		case  8: /* WAVE  */ return wave(obj);    
		case  9: /* CALM  */ return(2011); 
		case 10: /* WALK  */ return(2011); 
		case 11: /* ATTAC */ return attack(input, verb, obj);   
		case 12: /* POUR  */ return pour(obj);   
		case 13: /* EAT   */ return eat(obj);   
		case 14: /* DRINK */ return drink(obj);   
		case 15: /* RUB   */ return rub(obj);   
		case 16: /* TOSS  */ return throw(input, verb, obj);   
		case 17: /* QUIT  */ return(2011); 
		case 18: /* FIND  */ return find(obj);   
		case 19: /* INVEN */ return find(obj);   
		case 20: /* FEED  */ return feed(obj);   
		case 21: /* FILL  */ return fill(obj);   
		case 22: /* BLAST */ return blast();   
		case 23: /* SCOR  */ return(2011); 
		case 24: /* FOO   */ return(2011); 
		case 25: /* BRIEF */ return(2011); 
		case 26: /* READ  */ return read(input, obj);   
		case 27: /* BREAK */ return vbreak(obj);   
		case 28: /* WAKE  */ return wake(obj);   
		case 29: /* SUSP  */ return(2011); 
		case 30: /* RESU  */ return(2011); 
		case 31: /* FLY   */ return fly(obj);   
		case 32: /* LISTE */ return(2011); 
		case 33: /* ZZZZ  */ return reservoir();   
	    }
	    BUG(24);
	case unknown:
	    /*  Analyse an object word.  See if the thing is here, whether
	     *  we've got a verb yet, and so on.  Object must be here
	     *  unless verb is "find" or "invent(ory)" (and no new verb
	     *  yet to be analysed).  Water and oil are also funny, since
	     *  they are never actually dropped at any location, but might
	     *  be here inside the bottle or urn or as a feature of the
	     *  location. */
	    if (!HERE(obj))
		goto L5100;
	L5010:
	    if (WD2 > 0)
		return(2800);
	    if (verb != 0)
		goto L4090;
	    SETPRM(1,WD1,WD1X);
	    RSPEAK(255);
	    return(2600);

	L5100:
	    if (obj == GRATE) {
		if (game.loc == 1 || game.loc == 4 || game.loc == 7)
		    obj=DPRSSN;
		if (game.loc > 9 && game.loc < 15)
		    obj=ENTRNC;
		if (obj != GRATE)
		    return(8);
	    }

	    if (obj == DWARF && ATDWRF(game.loc) > 0)
		goto L5010;
	    if ((LIQ(0) == obj && HERE(BOTTLE)) || obj == LIQLOC(game.loc))
		goto L5010;
	    if (obj == OIL && HERE(URN) && game.prop[URN] != 0) {
		obj=URN;
		goto L5010;
	    }
	    if (obj == PLANT && AT(PLANT2) && game.prop[PLANT2] != 0) {
		obj=PLANT2;
		goto L5010;
	    }
	    if (obj == KNIFE && game.knfloc == game.loc) {
		game.knfloc= -1;
		SPK=116;
		return(2011);
	    }
	    if (obj == ROD && HERE(ROD2)) {
		obj=ROD2;
		goto L5010;
	    }
	    if ((verb == FIND || verb == INVENT) && WD2 <= 0)
		goto L5010;

	    SETPRM(1,WD1,WD1X);
	    RSPEAK(256);
	    return(2012);
	default:
	    BUG(99);
    }
}
