#include <stdbool.h>
#include "database.h"

/*  Statement functions
 *
 *  AT(OBJ)	= true if on either side of two-placed object
 *  CNDBIT(L,N) = true if COND(L) has bit n set (bit 0 is units bit)
 *  DARK(DUMMY) = true if location "LOC" is dark
 *  FORCED(LOC) = true if LOC moves without asking for input (COND=2)
 *  FOREST(LOC)  = true if LOC is part of the forest
 *  GSTONE(OBJ)  = true if OBJ is a gemstone
 *  HERE(OBJ)	= true if the OBJ is at "LOC" (or is being carried)
 *  LIQ(DUMMY)	= object number of liquid in bottle
 *  LIQLOC(LOC) = object number of liquid (if any) at LOC
 *  PCT(N)       = true N% of the time (N integer from 0 to 100)
 *  TOTING(OBJ) = true if the OBJ is being carried */

#define TOTING(OBJ)	(PLACE[OBJ] == -1)
#define AT(OBJ) (PLACE[OBJ] == LOC || FIXED[OBJ] == LOC)
#define HERE(OBJ)	(AT(OBJ) || TOTING(OBJ))
#define LIQ2(PBOTL)	((1-(PBOTL))*WATER+((PBOTL)/2)*(WATER+OIL))
#define LIQ(DUMMY)	(LIQ2(PROP[BOTTLE]<0 ? -1-PROP[BOTTLE] : PROP[BOTTLE]))
#define LIQLOC(LOC)	(LIQ2((MOD(COND[LOC]/2*2,8)-5)*MOD(COND[LOC]/4,2)+1))
#define CNDBIT(L,N)	(TSTBIT(COND[L],N))
#define FORCED(LOC)	(COND[LOC] == 2)
#define DARK(DUMMY)	((!CNDBIT(LOC,0)) && (PROP[LAMP] == 0 || !HERE(LAMP)))
#define PCT(N)	(randrange(100) < (N))
#define GSTONE(OBJ)	((OBJ) == EMRALD || (OBJ) == RUBY || (OBJ) == AMBER || (OBJ) == SAPPH)
#define FOREST(LOC)	((LOC) >= 145 && (LOC) <= 166)
#define VOCWRD(LETTRS,SECT)	(VOCAB(MAKEWD(LETTRS),SECT))

/*  The following two functions were added to fix a bug (CLOCK1 decremented
 *  while in forest).  They should probably be replaced by using another
 *  "cond" bit.  For now, however, a quick fix...  OUTSID(LOC) is true if
 *  LOC is outside, INDEEP(LOC) is true if LOC is "deep" in the cave (hall
 *  of mists or deeper).  Note special kludges for "Foof!" locs. */

#define OUTSID(LOC)	((LOC) <= 8 || FOREST(LOC) || (LOC) == PLAC[SAPPH] || (LOC) == 180 || (LOC) == 182)
#define INDEEP(LOC)	((LOC) >= 15 && !OUTSID(LOC) && (LOC) != 179)

extern int carry(void), discard(bool), attack(FILE *), throw(FILE *), feed(void), fill(void);
void score(long);




