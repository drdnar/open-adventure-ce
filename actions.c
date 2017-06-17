#include <stdlib.h>
#include <stdbool.h>
#include "advent.h"
#include "database.h"
#include "newdb.h"

/* Limit visibility of ugly globals.  Eventually these should go away. */
extern long WD1, WD1X, WD2, WD2X;

/*
 * Action handlers.  Eventually we'll do lookup through a method table
 * that calls these.
 */

static int fill(token_t verb, token_t);

static int attack(FILE *input, long verb, token_t obj)
/*  Attack.  Assume target if unambiguous.  "Throw" also links here.
 *  Attackable objects fall into two categories: enemies (snake,
 *  dwarf, etc.)  and others (bird, clam, machine).  Ambiguous if 2
 *  enemies, or no enemies but 2 others. */
{
    int spk = ACTSPK[verb];
    int d = ATDWRF(game.loc);
    if (obj == 0) {
	if (d > 0)
	    obj=DWARF;
	if (HERE(SNAKE))obj=obj*NOBJECTS+SNAKE;
	if (AT(DRAGON) && game.prop[DRAGON] == 0)obj=obj*NOBJECTS+DRAGON;
	if (AT(TROLL))obj=obj*NOBJECTS+TROLL;
	if (AT(OGRE))obj=obj*NOBJECTS+OGRE;
	if (HERE(BEAR) && game.prop[BEAR] == 0)obj=obj*NOBJECTS+BEAR;
	if (obj > NOBJECTS) return GO_UNKNOWN;
	if (obj == 0) {
	    /* Can't attack bird or machine by throwing axe. */
	    if (HERE(BIRD) && verb != THROW)obj=BIRD;
	    if (HERE(VEND) && verb != THROW)obj=obj*NOBJECTS+VEND;
	    /* Clam and oyster both treated as clam for intransitive case;
	     * no harm done. */
	    if (HERE(CLAM) || HERE(OYSTER))obj=NOBJECTS*obj+CLAM;
	    if (obj > NOBJECTS)
		return GO_UNKNOWN;
	}
    }
    if (obj == BIRD) {
	spk=UNHAPPY_BIRD;
	if (game.closed)
	{
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	DESTROY(BIRD);
	game.prop[BIRD]=0;
	spk=BIRD_DEAD;
    }
    else if (obj == VEND) {
	PSPEAK(VEND,game.prop[VEND]+2);
	game.prop[VEND]=3-game.prop[VEND];
	return GO_CLEAROBJ;
    }

    if (obj == 0)spk=NO_TARGET;
    if (obj == CLAM || obj == OYSTER)spk=SHELL_IMPERVIOUS;
    if (obj == SNAKE)spk=SNAKE_WARNING;
    if (obj == DWARF)spk=BARE_HANDS_QUERY;
    if (obj == DWARF && game.closed) return GO_DWARFWAKE;
    if (obj == DRAGON)spk=ALREADY_DEAD;
    if (obj == TROLL)spk=ROCKY_TROLL;
    if (obj == OGRE)spk=OGRE_DODGE;
    if (obj == OGRE && d > 0) {
	RSPEAK(spk);
	RSPEAK(KNIFE_THROWN);
	DESTROY(OGRE);
	int dwarves=0;
	for (int i=1; i < PIRATE; i++) {
	    if (game.dloc[i] == game.loc) {
		++dwarves;
		game.dloc[i] = LOC_LONGWEST;
		game.dseen[i]=false;
	    }
	}
	spk = (dwarves > 1) ? OGRE_PANIC1 : OGRE_PANIC2;
    }
    else if (obj == BEAR)
	/* FIXME: Arithmetic on message numbers */
	spk = BEAR_HANDS+(game.prop[BEAR]+1)/2;
    else if (obj == DRAGON && game.prop[DRAGON] == 0) {
	/*  Fun stuff for dragon.  If he insists on attacking it, win!
	 *  Set game.prop to dead, move dragon to central loc (still
	 *  fixed), move rug there (not fixed), and move him there,
	 *  too.  Then do a null motion to get new description. */
	RSPEAK(BARE_HANDS_QUERY);
	GETIN(input,&WD1,&WD1X,&WD2,&WD2X);
	if (WD1 != MAKEWD(25) && WD1 != MAKEWD(250519))
	    return GO_CHECKFOO;
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
	return GO_MOVE;
    }

    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int bigwords(long foo)
/*  FEE FIE FOE FOO (AND FUM).  Advance to next state if given in proper order.
 *  Look up WD1 in section 3 of vocab to determine which word we've got.  Last
 *  word zips the eggs back to the giant room (unless already there). */
{
    int k=VOCAB(foo,3);
    int spk=NOTHING_HAPPENS;
    if (game.foobar != 1-k) {
	if (game.foobar != 0)spk=START_OVER;
	RSPEAK(spk);
	return GO_CLEAROBJ;
    } else {
	game.foobar=k;
	if (k != 4) {
	    RSPEAK(OK_MAN);
	    return GO_CLEAROBJ;
	}
	game.foobar=0;
	if (game.place[EGGS]==PLAC[EGGS] || (TOTING(EGGS) && game.loc==PLAC[EGGS])) {
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	} else {
	    /*  Bring back troll if we steal the eggs back from him before
	     *  crossing. */
	    if (game.place[EGGS]==0 && game.place[TROLL]==0 && game.prop[TROLL]==0)
		game.prop[TROLL]=1;
	    k=2;
	    if (HERE(EGGS))k=1;
	    if (game.loc == PLAC[EGGS])k=0;
	    MOVE(EGGS,PLAC[EGGS]);
	    PSPEAK(EGGS,k);
	    return GO_CLEAROBJ;
	}
    }
}

static int bivalve(token_t verb, token_t obj)
/* Clam/oyster actions */
{
    int spk;
    bool is_oyster = (obj == OYSTER);
    spk= is_oyster ? OYSTER_OPENS : PEARL_FALLS;
    if (TOTING(obj))spk= is_oyster ? DROP_OYSTER : DROP_CLAM;
    if (!TOTING(TRIDNT))spk= is_oyster ? OYSTER_OPENER : CLAM_OPENER;
    if (verb == LOCK)spk=HUH_MAN;
    if (spk == PEARL_FALLS) {
	DESTROY(CLAM);
	DROP(OYSTER,game.loc);
	DROP(PEARL,LOC_CULDESAC);
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static void blast(void)
/*  Blast.  No effect unless you've got dynamite, which is a neat trick! */
{
    if (game.prop[ROD2] < 0 || !game.closed)
	RSPEAK(REQUIRES_DYNAMITE);
    else {
	game.bonus=VICTORY_MESSAGE;
	if (game.loc == LOC_NE)
	    game.bonus=DEFEAT_MESSAGE;
	if (HERE(ROD2))
	    game.bonus=SPLATTER_MESSAGE;
	RSPEAK(game.bonus);
	score(endgame);
    }
}

static int vbreak(token_t verb, token_t obj)
/*  Break.  Only works for mirror in repository and, of course, the vase. */
{
    int spk = ACTSPK[verb];
    if (obj == MIRROR)spk=TOO_FAR;
    if (obj == VASE && game.prop[VASE] == 0) {
	if (TOTING(VASE))DROP(VASE,game.loc);
	game.prop[VASE]=2;
	game.fixed[VASE]= -1;
	spk=BREAK_VASE;
    } else {
	if (obj == MIRROR && game.closed) {
	    RSPEAK(BREAK_MIRROR);
	    return GO_DWARFWAKE;
	}
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int brief(void)
/*  Brief.  Intransitive only.  Suppress long descriptions after first time. */
{
    game.abbnum=10000;
    game.detail=3;
    RSPEAK(BRIEF_CONFIRM);
    return GO_CLEAROBJ;
}

static int carry(token_t verb, token_t obj)
/*  Carry an object.  Special cases for bird and cage (if bird in cage, can't
 *  take one without the other).  Liquids also special, since they depend on
 *  status of bottle.  Also various side effects, etc. */
{
    int spk;
    if (obj == INTRANSITIVE) {
	/*  Carry, no object given yet.  OK if only one object present. */
	if(game.atloc[game.loc] == 0 ||
	   game.link[game.atloc[game.loc]] != 0 ||
	   ATDWRF(game.loc) > 0)
	    return GO_UNKNOWN;
	obj=game.atloc[game.loc];
    }

    if (TOTING(obj)) {RSPEAK(ALREADY_CARRYING); return GO_CLEAROBJ;}
    spk=YOU_JOKING;
    if (obj == PLANT && game.prop[PLANT] <= 0)spk=DEEP_ROOTS;
    if (obj == BEAR && game.prop[BEAR] == 1)spk=BEAR_CHAINED;
    if (obj == CHAIN && game.prop[BEAR] != 0)spk=STILL_LOCKED;
    if (obj == URN)spk=URN_NOBUDGE;
    if (obj == CAVITY)spk=DOUGHNUT_HOLES;
    if (obj == BLOOD)spk=FEW_DROPS;
    if (obj == RUG && game.prop[RUG] == 2)spk=RUG_HOVERS;
    if (obj == SIGN)spk=HAND_PASSTHROUGH;
    if (obj == MESSAG) {
	RSPEAK(REMOVE_MESSAGE);
	DESTROY(MESSAG);
	return GO_CLEAROBJ;
    }
    if (game.fixed[obj] != 0) {
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    if (obj == WATER || obj == OIL) {
	if (!HERE(BOTTLE) || LIQUID() != obj) {
	    if (TOTING(BOTTLE) && game.prop[BOTTLE] == 1)
		return(fill(verb, BOTTLE));
	    else {
		if (game.prop[BOTTLE] != 1)spk=BOTTLE_FULL;
		if (!TOTING(BOTTLE))spk=NO_CONTAINER;
		RSPEAK(spk);
		return GO_CLEAROBJ;
	    }
	}
	obj = BOTTLE;
    }

    spk=CARRY_LIMIT;
    if (game.holdng >= INVLIMIT) {
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    else if (obj == BIRD && game.prop[BIRD] != 1 && -1-game.prop[BIRD] != 1) {
	if (game.prop[BIRD] == 2) {
	    DESTROY(BIRD);
	    RSPEAK(BIRD_CRAP);
	    return GO_CLEAROBJ;
	}
	if (!TOTING(CAGE))spk=CANNOT_CARRY;
	if (TOTING(ROD))spk=BIRD_EVADES;
	if (spk == CANNOT_CARRY || spk == BIRD_EVADES) {
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	game.prop[BIRD]=1;
    }
    if ((obj==BIRD || obj==CAGE) && (game.prop[BIRD]==1 || -1-game.prop[BIRD]==1))
	CARRY(BIRD+CAGE-obj,game.loc);
    CARRY(obj,game.loc);
    if (obj == BOTTLE && LIQUID() != 0)
	game.place[LIQUID()] = CARRIED;
    if (GSTONE(obj) && game.prop[obj] != 0) {
	game.prop[obj]=0;
	game.prop[CAVITY]=1;
    }
    RSPEAK(OK_MAN);
    return GO_CLEAROBJ;
}

static int chain(token_t verb)
/* Do something to the bear's chain */
{
    int spk;
    if (verb != LOCK) {
	spk=CHAIN_UNLOCKED;
	if (game.prop[BEAR] == 0)spk=BEAR_BLOCKS;
	if (game.prop[CHAIN] == 0)spk=ALREADY_UNLOCKED;
	if (spk != CHAIN_UNLOCKED) {RSPEAK(spk); return GO_CLEAROBJ;}
	game.prop[CHAIN]=0;
	game.fixed[CHAIN]=0;
	if (game.prop[BEAR] != 3)game.prop[BEAR]=2;
	game.fixed[BEAR]=2-game.prop[BEAR];
    } else {
	spk=CHAIN_LOCKED;
	if (game.prop[CHAIN] != 0)spk=ALREADY_LOCKED;
	if (game.loc != PLAC[CHAIN])spk=NO_LOCKSITE;
	if (spk != CHAIN_LOCKED) {
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	game.prop[CHAIN]=2;
	if (TOTING(CHAIN))DROP(CHAIN,game.loc);
	game.fixed[CHAIN]= -1;
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int discard(token_t verb, token_t obj, bool just_do_it)
/*  Discard object.  "Throw" also comes here for most objects.  Special cases for
 *  bird (might attack snake or dragon) and cage (might contain bird) and vase.
 *  Drop coins at vending machine for extra batteries. */
{
    int spk = ACTSPK[verb];
    if (!just_do_it) {
        if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))obj=ROD2;
        if (!TOTING(obj)) {RSPEAK(spk); return GO_CLEAROBJ;}
        if (obj == BIRD && HERE(SNAKE)) {
            RSPEAK(BIRD_ATTACKS);
            if (game.closed) return GO_DWARFWAKE;
            DESTROY(SNAKE);
            /* Set game.prop for use by travel options */
            game.prop[SNAKE]=1;

        } else if ((GSTONE(obj) && AT(CAVITY) && game.prop[CAVITY] != 0)) {
            RSPEAK(GEM_FITS);
            game.prop[obj]=1;
            game.prop[CAVITY]=0;
            if (HERE(RUG) && ((obj == EMRALD && game.prop[RUG] != 2) || (obj == RUBY &&
									 game.prop[RUG] == 2))) {
                spk=RUG_RISES;
                if (TOTING(RUG))spk=RUG_WIGGLES;
                if (obj == RUBY)spk=RUG_SETTLES;
                RSPEAK(spk);
                if (spk != RUG_WIGGLES) {
                    int k = 2-game.prop[RUG];
                    game.prop[RUG] = k;
                    if (k == 2) k = PLAC[SAPPH];
                    MOVE(RUG+NOBJECTS, k);
                }
            }
        } else if (obj == COINS && HERE(VEND)) {
            DESTROY(COINS);
            DROP(BATTER,game.loc);
            PSPEAK(BATTER,0);
            return GO_CLEAROBJ;
        } else if (obj == BIRD && AT(DRAGON) && game.prop[DRAGON] == 0) {
            RSPEAK(BIRD_BURNT);
            DESTROY(BIRD);
            game.prop[BIRD]=0;
            return GO_CLEAROBJ;
        } else if (obj == BEAR && AT(TROLL)) {
            RSPEAK(TROLL_SCAMPERS);
            MOVE(TROLL,0);
            MOVE(TROLL+NOBJECTS,0);
            MOVE(TROLL2,PLAC[TROLL]);
            MOVE(TROLL2+NOBJECTS,FIXD[TROLL]);
            JUGGLE(CHASM);
            game.prop[TROLL]=2;
        } else if (obj != VASE || game.loc == PLAC[PILLOW]) {
            RSPEAK(OK_MAN);
        } else {
            game.prop[VASE]=2;
            if (AT(PILLOW))game.prop[VASE]=0;
            PSPEAK(VASE,game.prop[VASE]+1);
            if (game.prop[VASE] != 0)game.fixed[VASE]= -1;
        }
    }
    int k = LIQUID();
    if (k == obj)obj=BOTTLE;
    if (obj == BOTTLE && k != 0)
	game.place[k] = NOWHERE;
    if (obj == CAGE && game.prop[BIRD] == 1)DROP(BIRD,game.loc);
    DROP(obj,game.loc);
    if (obj != BIRD) return GO_CLEAROBJ;
    game.prop[BIRD]=0;
    if (FOREST(game.loc))game.prop[BIRD]=2;
    return GO_CLEAROBJ;
}

static int drink(token_t verb, token_t obj)
/*  Drink.  If no object, assume water and look for it here.  If water is in
 *  the bottle, drink that, else must be at a water loc, so drink stream. */
{
    int spk = ACTSPK[verb];
    if (obj == 0 && LIQLOC(game.loc) != WATER && (LIQUID() != WATER || !HERE(BOTTLE)))
	return GO_UNKNOWN;
    if (obj != BLOOD) {
	if (obj != 0 && obj != WATER)spk=RIDICULOUS_ATTEMPT;
	if (spk != RIDICULOUS_ATTEMPT && LIQUID() == WATER && HERE(BOTTLE)) {
	    game.prop[BOTTLE] = 1;
	    game.place[WATER] = NOWHERE;
	    spk=BOTTLE_EMPTY;
	}
    } else {
	DESTROY(BLOOD);
	game.prop[DRAGON]=2;
	OBJSND[BIRD]=OBJSND[BIRD]+3;
	spk=HEAD_BUZZES;
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int eat(token_t verb, token_t obj)
/*  Eat.  Intransitive: assume food if present, else ask what.  Transitive: food
 *  ok, some things lose appetite, rest are ridiculous. */
{
    int spk = ACTSPK[verb];
    if (obj == INTRANSITIVE) {
	if (!HERE(FOOD))
	    return GO_UNKNOWN;
	DESTROY(FOOD);
	spk=THANKS_DELICIOUS;
    } else {
	if (obj == FOOD) {
	    DESTROY(FOOD);
	    spk=THANKS_DELICIOUS;
	}
	if (obj == BIRD || obj == SNAKE || obj == CLAM || obj == OYSTER || obj ==
	   DWARF || obj == DRAGON || obj == TROLL || obj == BEAR || obj ==
	   OGRE)spk=LOST_APPETITE;
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int extinguish(token_t verb, int obj)
/* Extinguish.  Lamp, urn, dragon/volcano (nice try). */
{
    int spk = ACTSPK[verb];
    if (obj == INTRANSITIVE) {
	if (HERE(LAMP) && game.prop[LAMP] == 1)obj=LAMP;
	if (HERE(URN) && game.prop[URN] == 2)obj=obj*NOBJECTS+URN;
	if (obj == INTRANSITIVE || obj == 0 || obj > NOBJECTS) return GO_UNKNOWN;
    }

    if (obj == URN) {
	game.prop[URN]=game.prop[URN]/2;
	spk=URN_DARK;
    }
    else if (obj == LAMP) {
	game.prop[LAMP]=0;
	RSPEAK(LAMP_OFF);
	spk = DARK(game.loc) ? PITCH_DARK : NO_MESSAGE;
    }
    else if (obj == DRAGON || obj == VOLCAN)
	spk=BEYOND_POWER;
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int feed(token_t verb, token_t obj)
/*  Feed.  If bird, no seed.  Snake, dragon, troll: quip.  If dwarf, make him
 *  mad.  Bear, special. */
{
    int spk = ACTSPK[verb];
    if (obj == BIRD) {
	RSPEAK(BIRD_PINING);
	return GO_CLEAROBJ;
    }
    else if (obj == SNAKE || obj == DRAGON || obj == TROLL) {
	spk=NOTHING_EDIBLE;
	if (obj == DRAGON && game.prop[DRAGON] != 0)spk=RIDICULOUS_ATTEMPT;
	if (obj == TROLL)spk=TROLL_VICES;
	if (obj == SNAKE && !game.closed && HERE(BIRD)) {
	    DESTROY(BIRD);
	    game.prop[BIRD]=0;
	    spk = BIRD_DEVOURED;
	}
    }
    else if (obj == DWARF) {
	if (HERE(FOOD)) {
	    game.dflag += 2;
	    spk = REALLY_MAD;
	}
    }
    else if (obj == BEAR) {
	if (game.prop[BEAR] == 0)spk=NOTHING_EDIBLE;
	if (game.prop[BEAR] == 3)spk=RIDICULOUS_ATTEMPT;
	if (HERE(FOOD)) {
	    DESTROY(FOOD);
	    game.prop[BEAR]=1;
	    game.fixed[AXE]=0;
	    game.prop[AXE]=0;
	    spk=BEAR_TAMED;
	}
    }
    else if (obj == OGRE) {
	if (HERE(FOOD))
	    spk=OGRE_FULL;
    } else {
	spk=AM_GAME;
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

int fill(token_t verb, token_t obj)
/*  Fill.  Bottle or urn must be empty, and liquid available.  (Vase
 *  is nasty.) */
{
    int k;
    int spk = ACTSPK[verb];
    if (obj == VASE) {
	spk=ARENT_CARRYING;
	if (LIQLOC(game.loc) == 0)spk=FILL_INVALID;
	if (LIQLOC(game.loc) == 0 || !TOTING(VASE)) {
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	RSPEAK(SHATTER_VASE);
	game.prop[VASE] = 2;
	game.fixed[VASE]= -1;
	return(discard(verb, obj, true));
    }
    else if (obj == URN) {
	spk=FULL_URN;
	if (game.prop[URN] != 0) {RSPEAK(spk); return GO_CLEAROBJ;}
	spk=FILL_INVALID;
	k=LIQUID();
	if (k == 0 || !HERE(BOTTLE)) {RSPEAK(spk); return GO_CLEAROBJ;}
	game.place[k] = NOWHERE;
	game.prop[BOTTLE] = 1;
	if (k == OIL)game.prop[URN]=1;
	spk=WATER_URN+game.prop[URN];
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    else if (obj != 0 && obj != BOTTLE) {
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    else if (obj == 0 && !HERE(BOTTLE))
	return GO_UNKNOWN;
    spk=BOTTLED_WATER;
    if (LIQLOC(game.loc) == 0)
	spk=NO_LIQUID;
    if (HERE(URN) && game.prop[URN] != 0)
	spk=URN_NOPOUR;
    if (LIQUID() != 0)
	spk=BOTTLE_FULL;
    if (spk == 107) {
	game.prop[BOTTLE]=MOD(COND[game.loc],4)/2*2;
	k=LIQUID();
	if (TOTING(BOTTLE))
	    game.place[k] = CARRIED;
	if (k == OIL)
	    spk=BOTTLED_OIL;
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int find(token_t verb, token_t obj)
/* Find.  Might be carrying it, or it might be here.  Else give caveat. */
{
    int spk = ACTSPK[verb];
    if (AT(obj) ||
       (LIQUID() == obj && AT(BOTTLE)) ||
       obj == LIQLOC(game.loc) ||
       (obj == DWARF && ATDWRF(game.loc) > 0))
	spk=YOU_HAVEIT;
    if (game.closed)spk=NEEDED_NEreplace;
    if (TOTING(obj))spk=ALREADY_CARRYING;
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int fly(token_t verb, token_t obj)
/* Fly.  Snide remarks unless hovering rug is here. */
{
    int spk = ACTSPK[verb];
    if (obj == INTRANSITIVE) {
	if (game.prop[RUG] != 2)spk=RUG_NOTHING2;
	if (!HERE(RUG))spk=FLAP_ARMS;
	if (spk == RUG_NOTHING2 || spk == FLAP_ARMS) {
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	obj=RUG;
    }

    if (obj != RUG) {
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    spk=RUG_NOTHING1;
    if (game.prop[RUG] != 2) {RSPEAK(spk); return GO_CLEAROBJ;}
    game.oldlc2=game.oldloc;
    game.oldloc=game.loc;
    game.newloc=game.place[RUG]+game.fixed[RUG]-game.loc;
    spk=RUG_GOES;
    if (game.prop[SAPPH] >= 0)
	spk=RUG_RETURNS;
    RSPEAK(spk);
    return GO_TERMINATE;
}

static int inven(void)
/* Inventory. If object, treat same as find.  Else report on current burden. */
{
    int spk=NO_CARRY;
    for (int i=1; i<=NOBJECTS; i++) {
	if (i == BEAR || !TOTING(i))
	    continue;
	if (spk == NO_CARRY)
	    RSPEAK(NOW_HOLDING);
	game.blklin=false;
	PSPEAK(i,-1);
	game.blklin=true;
	spk=NO_MESSAGE;
    }
    if (TOTING(BEAR))
	spk=TAME_BEAR;
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int light(token_t verb, token_t obj)
/*  Light.  Applicable only to lamp and urn. */
{
    int spk = ACTSPK[verb];
    if (obj == INTRANSITIVE) {
	if (HERE(LAMP) && game.prop[LAMP] == 0 && game.limit >= 0)obj=LAMP;
	if (HERE(URN) && game.prop[URN] == 1)obj=obj*NOBJECTS+URN;
	if (obj == INTRANSITIVE || obj == 0 || obj > NOBJECTS) return GO_UNKNOWN;
    }

    if (obj == URN) {
	if (game.prop[URN] == 0) {
	    RSPEAK(URN_EMPTY);
	} else {
	    game.prop[URN] = 2;
	    RSPEAK(URN_LIT);
	}
	return GO_CLEAROBJ;
    } else {
	if (obj != LAMP)
	{
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	spk=LAMP_OUT;
	if (game.limit < 0) {
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	game.prop[LAMP]=1;
	RSPEAK(LAMP_ON);
	if (game.wzdark)
	    return GO_TOP;
	else
	    return GO_CLEAROBJ;
    }
}

static int listen(void)
/*  Listen.  Intransitive only.  Print stuff based on objsnd/locsnd. */
{
    int k;
    int spk=ALL_SILENT;
    k=LOCSND[game.loc];
    if (k != 0) {
	RSPEAK(labs(k));
	if (k < 0) return GO_CLEAROBJ;
	spk=NO_MESSAGE;
    }
    SETPRM(1,game.zzword,0);
    for (int i=1; i<=NOBJECTS; i++) {
	if (!HERE(i) || OBJSND[i] == 0 || game.prop[i] < 0)
	    continue;
	PSPEAK(i,OBJSND[i]+game.prop[i]);
	spk=NO_MESSAGE;
	if (i == BIRD && OBJSND[i]+game.prop[i] == 8)
	    DESTROY(BIRD);
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int lock(token_t verb, token_t obj)
/* Lock, unlock, no object given.  Assume various things if present. */
{
    int spk = ACTSPK[verb];
    if (obj == INTRANSITIVE) {
	spk=NOTHING_LOCKED;
	if (HERE(CLAM))obj=CLAM;
	if (HERE(OYSTER))obj=OYSTER;
	if (AT(DOOR))obj=DOOR;
	if (AT(GRATE))obj=GRATE;
	if (obj != 0 && HERE(CHAIN)) return GO_UNKNOWN;
	if (HERE(CHAIN))obj=CHAIN;
	if (obj == 0) {RSPEAK(spk); return GO_CLEAROBJ;}
    }
	
    /*  Lock, unlock object.  Special stuff for opening clam/oyster
     *  and for chain. */
    if (obj == CLAM || obj == OYSTER)
	return bivalve(verb, obj);
    if (obj == DOOR)spk=RUSTY_DOOR;
    if (obj == DOOR && game.prop[DOOR] == 1)spk=OK_MAN;
    if (obj == CAGE)spk=NO_LOCK;
    if (obj == KEYS)spk=CANNOT_UNLOCK;
    if (obj == GRATE || obj == CHAIN) {
	spk=NO_KEYS;
	if (HERE(KEYS)) {
	    if (obj == CHAIN)
		return chain(verb);
	    if (game.closng) {
		spk=EXIT_CLOSED;
		if (!game.panic)game.clock2=15;
		game.panic=true;
	    } else {
		spk=game.prop[GRATE] ? GRATE_LOCKED : ALREADY_LOCKED;
		game.prop[GRATE]=1;
		if (verb == LOCK)game.prop[GRATE]=0;
		spk=game.prop[GRATE] ? GRATE_UNLOCKED : GRATE_LOCKED;
	    }
	}
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int pour(token_t verb, token_t obj)
/*  Pour.  If no object, or object is bottle, assume contents of bottle.
 *  special tests for pouring water or oil on plant or rusty door. */
{
    int spk = ACTSPK[verb];
    if (obj == BOTTLE || obj == 0)obj=LIQUID();
    if (obj == 0) return GO_UNKNOWN;
    if (!TOTING(obj)) {RSPEAK(spk); return GO_CLEAROBJ;}
    spk=CANT_POUR;
    if (obj != OIL && obj != WATER) {RSPEAK(spk); return GO_CLEAROBJ;}
    if (HERE(URN) && game.prop[URN] == 0)
	return fill(verb, URN);
    game.prop[BOTTLE] = 1;
    game.place[obj] = NOWHERE;
    spk=GROUND_WET;
    if (!(AT(PLANT) || AT(DOOR)))
	{RSPEAK(spk); return GO_CLEAROBJ;}
    if (!AT(DOOR)) {
	spk=SHAKING_LEAVES;
	if (obj != WATER) {RSPEAK(spk); return GO_CLEAROBJ;}
	PSPEAK(PLANT,game.prop[PLANT]+3);
	game.prop[PLANT]=MOD(game.prop[PLANT]+1,3);
	game.prop[PLANT2]=game.prop[PLANT];
	return GO_MOVE;
    } else {
	game.prop[DOOR]=0;
	if (obj == OIL)game.prop[DOOR]=1;
	spk=RUSTED_HINGES+game.prop[DOOR];
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
}

static int quit(FILE *input)
/*  Quit.  Intransitive only.  Verify intent and exit if that's what he wants. */
{
    if (YES(input,REALLY_QUIT,OK_MAN,OK_MAN))
	score(quitgame);
    return GO_CLEAROBJ;
}

static int read(FILE *input, token_t verb, token_t obj)
/*  Read.  Print stuff based on objtxt.  Oyster (?) is special case. */
{
    int spk = ACTSPK[verb];
    if (obj == INTRANSITIVE) {
	obj = 0;
	for (int i=1; i<=NOBJECTS; i++) {
	    if (HERE(i) && OBJTXT[i] != 0 && game.prop[i] >= 0)
		obj = obj * NOBJECTS + i;
	}
	if (obj > NOBJECTS || obj == 0 || DARK(game.loc)) return GO_UNKNOWN;
    }
	
    if (DARK(game.loc)) {
	SETPRM(1,WD1,WD1X);
	RSPEAK(NO_SEE);
	return GO_CLEAROBJ;
    }
    if (OBJTXT[obj] == 0 || game.prop[obj] < 0) {
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    if (obj == OYSTER && !game.clshnt) {
	game.clshnt=YES(input,CLUE_QUERY,WAYOUT_CLUE,OK_MAN);
	return GO_CLEAROBJ;
    }
    PSPEAK(obj,OBJTXT[obj]+game.prop[obj]);
    return GO_CLEAROBJ;
}

static int reservoir(void)
/*  Z'ZZZ (word gets recomputed at startup; different each game). */
{
    if (!AT(RESER) && game.loc != game.fixed[RESER]-1) {
	RSPEAK(NOTHING_HAPPENS);
	return GO_CLEAROBJ;
    } else {
	PSPEAK(RESER,game.prop[RESER]+1);
	game.prop[RESER]=1-game.prop[RESER];
	if (AT(RESER))
	    return GO_CLEAROBJ;
	else {
	    game.oldlc2=game.loc;
	    game.newloc=0;
	    RSPEAK(NOT_BRIGHT);
	    return GO_TERMINATE;
	}
    }
}

static int rub(token_t verb, token_t obj)
/* Rub.  Yields various snide remarks except for lit urn. */
{
    int spk = ACTSPK[verb];
    if (obj != LAMP)
	spk=PECULIAR_NOTHING;
    if (obj == URN && game.prop[URN] == 2) {
	DESTROY(URN);
	DROP(AMBER,game.loc);
	game.prop[AMBER]=1;
	--game.tally;
	DROP(CAVITY,game.loc);
	spk=URN_GENIES;
    }
    RSPEAK(spk);
    return GO_CLEAROBJ;
}

static int say(void)
/* Say.  Echo WD2 (or WD1 if no WD2 (SAY WHAT?, etc.).)  Magic words override. */
{
    /* FIXME: ugly use of globals */
    SETPRM(1,WD2,WD2X);
    if (WD2 <= 0)
	SETPRM(1,WD1,WD1X);
    if (WD2 > 0)
	WD1=WD2;
    int wd=VOCAB(WD1,-1);
    /* FIXME: Magic numbers */
    if (wd == 62 || wd == 65 || wd == 71 || wd == 2025 || wd == 2034) {
	WD2=0;
	return GO_LOOKUP;
    }
    RSPEAK(OKEY_DOKEY);
    return GO_CLEAROBJ;
}

static int throw_support(long spk)
{
    RSPEAK(spk);
    DROP(AXE,game.loc);
    return GO_MOVE;
}

static int throw(FILE *cmdin, long verb, token_t obj)
/*  Throw.  Same as discard unless axe.  Then same as attack except
 *  ignore bird, and if dwarf is present then one might be killed.
 *  (Only way to do so!)  Axe also special for dragon, bear, and
 *  troll.  Treasures special for troll. */
{
    int spk = ACTSPK[verb];
    if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))obj=ROD2;
    if (!TOTING(obj)) {
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    if (obj >= MINTRS && obj <= MAXTRS && AT(TROLL)) {
        spk=TROLL_SATISFIED;
        /*  Snarf a treasure for the troll. */
        DROP(obj,0);
        MOVE(TROLL,0);
        MOVE(TROLL+NOBJECTS,0);
        DROP(TROLL2,PLAC[TROLL]);
        DROP(TROLL2+NOBJECTS,FIXD[TROLL]);
        JUGGLE(CHASM);
        RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    if (obj == FOOD && HERE(BEAR)) {
    /* But throwing food is another story. */
        obj=BEAR;
        return(feed(verb, obj));
    }
    if (obj != AXE)
	return(discard(verb, obj, false));
    int i=ATDWRF(game.loc);
    if (i <= 0) {
        if (AT(DRAGON) && game.prop[DRAGON] == 0) {
            spk=DRAGON_SCALES;
            return throw_support(spk);
        }
        if (AT(TROLL)) {
            spk=TROLL_RETURNS;
            return throw_support(spk);
        }
        if (AT(OGRE)) {
            spk=OGRE_DODGE;
            return throw_support(spk);
        }
        if (HERE(BEAR) && game.prop[BEAR] == 0) {
            /* This'll teach him to throw the axe at the bear! */
            DROP(AXE,game.loc);
            game.fixed[AXE] = -1;
            game.prop[AXE] = 1;
            JUGGLE(BEAR);
            RSPEAK(AXE_LOST);
	    return GO_CLEAROBJ;
        }
        return(attack(cmdin, verb, 0));
    }

    if (randrange(NDWARVES+1) < game.dflag) {
        return throw_support(DWARF_DODGES);
    }
    game.dseen[i]=false;
    game.dloc[i]=0;
    return throw_support((++game.dkill == 1) ? DWARF_SMOKE : KILLED_DWARF);
}

static int vscore(void)
/* Score.  Call scoring routine but tell it to return. */
{
    score(scoregame);
    return GO_CLEAROBJ;
}

static int wake(token_t verb, token_t obj)
/* Wake.  Only use is to disturb the dwarves. */
{
    if (obj != DWARF || !game.closed) {
	RSPEAK(ACTSPK[verb]);
	return GO_CLEAROBJ;
    } else {
	RSPEAK(PROD_DWARF);
	return GO_DWARFWAKE;
    }
}

static int wave(token_t verb, token_t obj)
/* Wave.  No effect unless waving rod at fissure or at bird. */
{
    int spk = ACTSPK[verb];
    if ((!TOTING(obj)) && (obj != ROD || !TOTING(ROD2)))spk=ARENT_CARRYING;
    if (obj != ROD ||
       !TOTING(obj) ||
       (!HERE(BIRD) && (game.closng || !AT(FISSUR)))) {
	RSPEAK(spk);
	return GO_CLEAROBJ;
    }
    if (HERE(BIRD))spk=FREE_FLY+MOD(game.prop[BIRD],2);
    if (spk == 206 && game.loc == game.place[STEPS] && game.prop[JADE] < 0) {
	DROP(JADE,game.loc);
	game.prop[JADE]=0;
	--game.tally;
	spk=NECKLACE_FLY;
	RSPEAK(spk);
	return GO_CLEAROBJ;
    } else {
	if (game.closed) {
	    RSPEAK(spk);
	    return GO_DWARFWAKE;
	}
	if (game.closng || !AT(FISSUR)) {RSPEAK(spk); return GO_CLEAROBJ;}
	if (HERE(BIRD))RSPEAK(spk);
	game.prop[FISSUR]=1-game.prop[FISSUR];
	PSPEAK(FISSUR,2-game.prop[FISSUR]);
	return GO_CLEAROBJ;
    }
}

int action(FILE *input, enum speechpart part, long verb, token_t obj)
/*  Analyse a verb.  Remember what it was, go back for object if second word
 *  unless verb is "say", which snarfs arbitrary second word.
 */
{
    token_t spk=ACTSPK[verb];

    if (part == unknown)
    {
	/*  Analyse an object word.  See if the thing is here, whether
	 *  we've got a verb yet, and so on.  Object must be here
	 *  unless verb is "find" or "invent(ory)" (and no new verb
	 *  yet to be analysed).  Water and oil are also funny, since
	 *  they are never actually dropped at any location, but might
	 *  be here inside the bottle or urn or as a feature of the
	 *  location. */
	if (HERE(obj))
	    /* FALL THROUGH */;
	else if (obj == GRATE) {
	    if (game.loc == LOC_START || game.loc == LOC_VALLEY || game.loc == LOC_SLIT)
		obj=DPRSSN;
	    if (game.loc == LOC_COBBLE || game.loc == LOC_DEBRIS || game.loc == LOC_AWKWARD ||
		game.loc == LOC_BIRD || game.loc == LOC_PITTOP)
		obj=ENTRNC;
	    if (obj != GRATE)
		return GO_MOVE;
	}
	else if (obj == DWARF && ATDWRF(game.loc) > 0)
	    /* FALL THROUGH */;
	else if ((LIQUID() == obj && HERE(BOTTLE)) || obj == LIQLOC(game.loc))
	    /* FALL THROUGH */;
	else if (obj == OIL && HERE(URN) && game.prop[URN] != 0) {
	    obj=URN;
	    /* FALL THROUGH */;
	}
	else if (obj == PLANT && AT(PLANT2) && game.prop[PLANT2] != 0) {
	    obj=PLANT2;
	    /* FALL THROUGH */;
	}
	else if (obj == KNIFE && game.knfloc == game.loc) {
	    game.knfloc= -1;
	    spk=KNIVES_VANISH;
	    RSPEAK(spk);
	    return GO_CLEAROBJ;
	}
	else if (obj == ROD && HERE(ROD2)) {
	    obj=ROD2;
	    /* FALL THROUGH */;
	}
	else if ((verb == FIND || verb == INVENT) && WD2 <= 0)
	    /* FALL THROUGH */;
	else {
	    SETPRM(1,WD1,WD1X);
	    RSPEAK(NO_SEE);
	    return GO_CLEAROBJ;
	}

	if (WD2 > 0)
	    return GO_WORD2;
	if (verb != 0)
	    part = transitive;
    }

    switch(part)
    {
	case intransitive:
	    if (WD2 > 0 && verb != SAY) return(2800);
	    if (verb == SAY)obj=WD2;
	    if (obj == 0 || obj == INTRANSITIVE) {
		/*  Analyse an intransitive verb (ie, no object given yet). */
		    switch (verb-1) {
		    case  0: /* CARRY */ return carry(verb, INTRANSITIVE);
		    case  1: /* DROP  */ return GO_UNKNOWN; 
		    case  2: /* SAY   */ return GO_UNKNOWN; 
		    case  3: /* UNLOC */ return lock(verb, INTRANSITIVE);    
		    case  4: /* NOTHI */ {RSPEAK(OK_MAN); return(GO_CLEAROBJ);}
		    case  5: /* LOCK  */ return lock(verb, INTRANSITIVE);    
		    case  6: /* LIGHT */ return light(verb, INTRANSITIVE);    
		    case  7: /* EXTIN */ return extinguish(verb, INTRANSITIVE);    
		    case  8: /* WAVE  */ return GO_UNKNOWN; 
		    case  9: /* CALM  */ return GO_UNKNOWN; 
		    case 10: /* WALK  */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		    case 11: /* ATTAC */ return attack(input, verb, obj);   
		    case 12: /* POUR  */ return pour(verb, obj);   
		    case 13: /* EAT   */ return eat(verb, INTRANSITIVE);   
		    case 14: /* DRINK */ return drink(verb, obj);   
		    case 15: /* RUB   */ return GO_UNKNOWN; 
		    case 16: /* TOSS  */ return GO_UNKNOWN; 
		    case 17: /* QUIT  */ return quit(input);   
		    case 18: /* FIND  */ return GO_UNKNOWN; 
		    case 19: /* INVEN */ return inven();
		    case 20: /* FEED  */ return GO_UNKNOWN; 
		    case 21: /* FILL  */ return fill(verb, obj);   
		    case 22: /* BLAST */ blast(); return GO_CLEAROBJ;
		    case 23: /* SCOR  */ return vscore();   
		    case 24: /* FOO   */ return bigwords(WD1);   
		    case 25: /* BRIEF */ return brief();   
		    case 26: /* READ  */ return read(input, verb, INTRANSITIVE);
		    case 27: /* BREAK */ return GO_UNKNOWN; 
		    case 28: /* WAKE  */ return GO_UNKNOWN; 
		    case 29: /* SUSP  */ return suspend(input);
		    case 30: /* RESU  */ return resume(input);
		    case 31: /* FLY   */ return fly(verb, INTRANSITIVE);   
		    case 32: /* LISTE */ return listen();   
		    case 33: /* ZZZZ  */ return reservoir();   
		}
		BUG(23);
	    }
	    /* FALLTHRU */
	case transitive:
	    /*  Analyse a transitive verb. */
	    switch (verb-1) {
	    case  0: /* CARRY */ return carry(verb, obj);    
	        case  1: /* DROP  */ return discard(verb, obj, false);    
		case  2: /* SAY   */ return say();    
		case  3: /* UNLOC */ return lock(verb, obj);    
		case  4: /* NOTHI */ {RSPEAK(OK_MAN); return(GO_CLEAROBJ);}
		case  5: /* LOCK  */ return lock(verb, obj);    
		case  6: /* LIGHT */ return light(verb, obj);    
		case  7: /* EXTI  */ return extinguish(verb, obj);    
		case  8: /* WAVE  */ return wave(verb, obj);    
		case  9: /* CALM  */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 10: /* WALK  */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 11: /* ATTAC */ return attack(input, verb, obj);   
		case 12: /* POUR  */ return pour(verb, obj);   
       		case 13: /* EAT   */ return eat(verb, obj);   
		case 14: /* DRINK */ return drink(verb, obj);   
		case 15: /* RUB   */ return rub(verb, obj);   
		case 16: /* TOSS  */ return throw(input, verb, obj);   
		case 17: /* QUIT  */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 18: /* FIND  */ return find(verb, obj);   
		case 19: /* INVEN */ return find(verb, obj);   
		case 20: /* FEED  */ return feed(verb, obj);   
		case 21: /* FILL  */ return fill(verb, obj);   
		case 22: /* BLAST */ blast(); return GO_CLEAROBJ;
		case 23: /* SCOR  */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 24: /* FOO   */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 25: /* BRIEF */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 26: /* READ  */ return read(input, verb, obj);   
		case 27: /* BREAK */ return vbreak(verb, obj);   
		case 28: /* WAKE  */ return wake(verb, obj);   
		case 29: /* SUSP  */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 30: /* RESU  */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 31: /* FLY   */ return fly(verb, obj);   
		case 32: /* LISTE */ {RSPEAK(spk); return GO_CLEAROBJ;} 
		case 33: /* ZZZZ  */ return reservoir();   
	    }
	    BUG(24);
	case unknown:
	    /* Unknown verb, couldn't deduce object - might need hint */
	    SETPRM(1,WD1,WD1X);
	    RSPEAK(WHAT_DO);
	    return GO_CHECKHINT;
    default:
	    BUG(99);
    }
}
