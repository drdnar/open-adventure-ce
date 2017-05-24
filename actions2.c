#include "misc.h"
#include "main.h"
#include "share.h"
#include "funcs.h"

/*  Carry an object.  Special cases for bird and cage (if bird in cage, can't
 *  take one without the other).  Liquids also special, since they depend on
 *  status of bottle.  Also various side effects, etc. */

int carry(void) {
	if(TOTING(OBJ)) return(2011);
	game.spk=25;
	if(OBJ == PLANT && PROP[PLANT] <= 0)game.spk=115;
	if(OBJ == BEAR && PROP[BEAR] == 1)game.spk=169;
	if(OBJ == CHAIN && PROP[BEAR] != 0)game.spk=170;
	if(OBJ == URN)game.spk=215;
	if(OBJ == CAVITY)game.spk=217;
	if(OBJ == BLOOD)game.spk=239;
	if(OBJ == RUG && PROP[RUG] == 2)game.spk=222;
	if(OBJ == SIGN)game.spk=196;
	if(OBJ != MESSAG) goto L9011;
	game.spk=190;
	DSTROY(MESSAG);
L9011:	if(FIXED[OBJ] != 0) return(2011);
	if(OBJ != WATER && OBJ != OIL) goto L9017;
	K=OBJ;
	OBJ=BOTTLE;
	if(HERE(BOTTLE) && LIQ(0) == K) goto L9017;
	if(TOTING(BOTTLE) && PROP[BOTTLE] == 1) return(fill());
	if(PROP[BOTTLE] != 1)game.spk=105;
	if(!TOTING(BOTTLE))game.spk=104;
	 return(2011);
L9017:	game.spk=92;
	if(game.holdng >= 7) return(2011);
	if(OBJ != BIRD || PROP[BIRD] == 1 || -1-PROP[BIRD] == 1) goto L9014;
	if(PROP[BIRD] == 2) goto L9015;
	if(!TOTING(CAGE))game.spk=27;
	if(TOTING(ROD))game.spk=26;
	if(game.spk/2 == 13) return(2011);
	PROP[BIRD]=1;
L9014:	if((OBJ == BIRD || OBJ == CAGE) && (PROP[BIRD] == 1 || -1-PROP[BIRD] ==
		1))CARRY(BIRD+CAGE-OBJ,LOC);
	CARRY(OBJ,LOC);
	K=LIQ(0);
	if(OBJ == BOTTLE && K != 0)PLACE[K]= -1;
	if(!GSTONE(OBJ) || PROP[OBJ] == 0) return(2009);
	PROP[OBJ]=0;
	PROP[CAVITY]=1;
	 return(2009);

L9015:	game.spk=238;
	DSTROY(BIRD);
	 return(2011);
}

/*  Discard object.  "Throw" also comes here for most objects.  Special cases for
 *  bird (might attack snake or dragon) and cage (might contain bird) and vase.
 *  Drop coins at vending machine for extra batteries. */

int discard(bool just_do_it) {
	if(just_do_it) goto L9021;
	if(TOTING(ROD2) && OBJ == ROD && !TOTING(ROD))OBJ=ROD2;
	if(!TOTING(OBJ)) return(2011);
	if(OBJ != BIRD || !HERE(SNAKE)) goto L9023;
	RSPEAK(30);
	if(game.closed) return(19000);
	DSTROY(SNAKE);
/*  SET PROP FOR USE BY TRAVEL OPTIONS */
	PROP[SNAKE]=1;
L9021:	K=LIQ(0);
	if(K == OBJ)OBJ=BOTTLE;
	if(OBJ == BOTTLE && K != 0)PLACE[K]=0;
	if(OBJ == CAGE && PROP[BIRD] == 1)DROP(BIRD,LOC);
	DROP(OBJ,LOC);
	if(OBJ != BIRD) return(2012);
	PROP[BIRD]=0;
	if(FOREST(LOC))PROP[BIRD]=2;
	 return(2012);

L9023:	if(!(GSTONE(OBJ) && AT(CAVITY) && PROP[CAVITY] != 0)) goto L9024;
	RSPEAK(218);
	PROP[OBJ]=1;
	PROP[CAVITY]=0;
	if(!HERE(RUG) || !((OBJ == EMRALD && PROP[RUG] != 2) || (OBJ == RUBY &&
		PROP[RUG] == 2))) goto L9021;
	game.spk=219;
	if(TOTING(RUG))game.spk=220;
	if(OBJ == RUBY)game.spk=221;
	RSPEAK(game.spk);
	if(game.spk == 220) goto L9021;
	K=2-PROP[RUG];
	PROP[RUG]=K;
	if(K == 2)K=PLAC[SAPPH];
	MOVE(RUG+100,K);
	 goto L9021;

L9024:	if(OBJ != COINS || !HERE(VEND)) goto L9025;
	DSTROY(COINS);
	DROP(BATTER,LOC);
	PSPEAK(BATTER,0);
	 return(2012);

L9025:	if(OBJ != BIRD || !AT(DRAGON) || PROP[DRAGON] != 0) goto L9026;
	RSPEAK(154);
	DSTROY(BIRD);
	PROP[BIRD]=0;
	 return(2012);

L9026:	if(OBJ != BEAR || !AT(TROLL)) goto L9027;
	RSPEAK(163);
	MOVE(TROLL,0);
	MOVE(TROLL+100,0);
	MOVE(TROLL2,PLAC[TROLL]);
	MOVE(TROLL2+100,FIXD[TROLL]);
	JUGGLE(CHASM);
	PROP[TROLL]=2;
	 goto L9021;

L9027:	if(OBJ == VASE && LOC != PLAC[PILLOW]) goto L9028;
	RSPEAK(54);
	 goto L9021;

L9028:	PROP[VASE]=2;
	if(AT(PILLOW))PROP[VASE]=0;
	PSPEAK(VASE,PROP[VASE]+1);
	if(PROP[VASE] != 0)FIXED[VASE]= -1;
	 goto L9021;
}

/*  Attack.  Assume target if unambiguous.  "Throw" also links here.  Attackable
 *  objects fall into two categories: enemies (snake, dwarf, etc.)  and others
 *  (bird, clam, machine).  Ambiguous if 2 enemies, or no enemies but 2 others. */

int attack(FILE *input) {
	I=ATDWRF(LOC);
	if(OBJ != 0) goto L9124;
	if(I > 0)OBJ=DWARF;
	if(HERE(SNAKE))OBJ=OBJ*100+SNAKE;
	if(AT(DRAGON) && PROP[DRAGON] == 0)OBJ=OBJ*100+DRAGON;
	if(AT(TROLL))OBJ=OBJ*100+TROLL;
	if(AT(OGRE))OBJ=OBJ*100+OGRE;
	if(HERE(BEAR) && PROP[BEAR] == 0)OBJ=OBJ*100+BEAR;
	if(OBJ > 100) return(8000);
	if(OBJ != 0) goto L9124;
/*  CAN'T ATTACK BIRD OR MACHINE BY THROWING AXE. */
	if(HERE(BIRD) && VERB != THROW)OBJ=BIRD;
	if(HERE(VEND) && VERB != THROW)OBJ=OBJ*100+VEND;
/*  CLAM AND OYSTER BOTH TREATED AS CLAM FOR INTRANSITIVE CASE; NO HARM DONE. */
	if(HERE(CLAM) || HERE(OYSTER))OBJ=100*OBJ+CLAM;
	if(OBJ > 100) return(8000);
L9124:	if(OBJ != BIRD) goto L9125;
	game.spk=137;
	if(game.closed) return(2011);
	DSTROY(BIRD);
	PROP[BIRD]=0;
	game.spk=45;
L9125:	if(OBJ != VEND) goto L9126;
	PSPEAK(VEND,PROP[VEND]+2);
	PROP[VEND]=3-PROP[VEND];
	 return(2012);

L9126:	if(OBJ == 0)game.spk=44;
	if(OBJ == CLAM || OBJ == OYSTER)game.spk=150;
	if(OBJ == SNAKE)game.spk=46;
	if(OBJ == DWARF)game.spk=49;
	if(OBJ == DWARF && game.closed) return(19000);
	if(OBJ == DRAGON)game.spk=167;
	if(OBJ == TROLL)game.spk=157;
	if(OBJ == OGRE)game.spk=203;
	if(OBJ == OGRE && I > 0) goto L9128;
	if(OBJ == BEAR)game.spk=165+(PROP[BEAR]+1)/2;
	if(OBJ != DRAGON || PROP[DRAGON] != 0) return(2011);
/*  Fun stuff for dragon.  If he insists on attacking it, win!  Set PROP to dead,
 *  move dragon to central loc (still fixed), move rug there (not fixed), and
 *  move him there, too.  Then do a null motion to get new description. */
	RSPEAK(49);
	VERB=0;
	OBJ=0;
	GETIN(input,WD1,WD1X,WD2,WD2X);
	if(WD1 != MAKEWD(25) && WD1 != MAKEWD(250519)) return(2607);
	PSPEAK(DRAGON,3);
	PROP[DRAGON]=1;
	PROP[RUG]=0;
	K=(PLAC[DRAGON]+FIXD[DRAGON])/2;
	MOVE(DRAGON+100,-1);
	MOVE(RUG+100,0);
	MOVE(DRAGON,K);
	MOVE(RUG,K);
	DROP(BLOOD,K);
	for (OBJ=1; OBJ<=100; OBJ++) {
	if(PLACE[OBJ] == PLAC[DRAGON] || PLACE[OBJ] == FIXD[DRAGON])MOVE(OBJ,K);
	/*etc*/ ;
	} /* end loop */
	LOC=K;
	K=NUL;
	 return(8);

L9128:	RSPEAK(game.spk);
	RSPEAK(6);
	DSTROY(OGRE);
	K=0;
	/* 9129 */ for (I=1; I<=5; I++) {
	if(DLOC[I] != LOC) goto L9129;
	K=K+1;
	DLOC[I]=61;
	game.dseen[I]=false;
L9129:	/*etc*/ ;
	} /* end loop */
	game.spk=game.spk+1+1/K;
	 return(2011);
}

/*  Throw.  Same as discard unless axe.  Then same as attack except ignore bird,
 *  and if dwarf is present then one might be killed.  (Only way to do so!)
 *  Axe also special for dragon, bear, and troll.  Treasures special for troll. */

int throw(FILE *cmdin) {
	if(TOTING(ROD2) && OBJ == ROD && !TOTING(ROD))OBJ=ROD2;
	if(!TOTING(OBJ)) return(2011);
	if(OBJ >= 50 && OBJ <= MAXTRS && AT(TROLL)) goto L9178;
	if(OBJ == FOOD && HERE(BEAR)) goto L9177;
	if(OBJ != AXE) return(discard(false));
	I=ATDWRF(LOC);
	if(I > 0) goto L9172;
	game.spk=152;
	if(AT(DRAGON) && PROP[DRAGON] == 0) goto L9175;
	game.spk=158;
	if(AT(TROLL)) goto L9175;
	game.spk=203;
	if(AT(OGRE)) goto L9175;
	if(HERE(BEAR) && PROP[BEAR] == 0) goto L9176;
	OBJ=0;
	return(attack(cmdin));

L9172:	game.spk=48;
	if(RAN(7) < game.dflag) goto L9175;
	game.dseen[I]=false;
	DLOC[I]=0;
	game.spk=47;
	game.dkill=game.dkill+1;
	if(game.dkill == 1)game.spk=149;
L9175:	RSPEAK(game.spk);
	DROP(AXE,LOC);
	K=NUL;
	 return(8);

/*  This'll teach him to throw the axe at the bear! */
L9176:	game.spk=164;
	DROP(AXE,LOC);
	FIXED[AXE]= -1;
	PROP[AXE]=1;
	JUGGLE(BEAR);
	 return(2011);

/*  But throwing food is another story. */
L9177:	OBJ=BEAR;
	return(feed());

L9178:	game.spk=159;
/*  Snarf a treasure for the troll. */
	DROP(OBJ,0);
	MOVE(TROLL,0);
	MOVE(TROLL+100,0);
	DROP(TROLL2,PLAC[TROLL]);
	DROP(TROLL2+100,FIXD[TROLL]);
	JUGGLE(CHASM);
	 return(2011);
}

/*  Feed.  If bird, no seed.  Snake, dragon, troll: quip.  If dwarf, make him
 *  mad.  Bear, special. */

int feed() {
	if(OBJ != BIRD) goto L9212;
	game.spk=100;
	 return(2011);

L9212:	if(OBJ != SNAKE && OBJ != DRAGON && OBJ != TROLL) goto L9213;
	game.spk=102;
	if(OBJ == DRAGON && PROP[DRAGON] != 0)game.spk=110;
	if(OBJ == TROLL)game.spk=182;
	if(OBJ != SNAKE || game.closed || !HERE(BIRD)) return(2011);
	game.spk=101;
	DSTROY(BIRD);
	PROP[BIRD]=0;
	 return(2011);

L9213:	if(OBJ != DWARF) goto L9214;
	if(!HERE(FOOD)) return(2011);
	game.spk=103;
	game.dflag=game.dflag+2;
	 return(2011);

L9214:	if(OBJ != BEAR) goto L9215;
	if(PROP[BEAR] == 0)game.spk=102;
	if(PROP[BEAR] == 3)game.spk=110;
	if(!HERE(FOOD)) return(2011);
	DSTROY(FOOD);
	PROP[BEAR]=1;
	FIXED[AXE]=0;
	PROP[AXE]=0;
	game.spk=168;
	 return(2011);

L9215:	if(OBJ != OGRE) goto L9216;
	if(HERE(FOOD))game.spk=202;
	 return(2011);

L9216:	game.spk=14;
	 return(2011);
}

/*  Fill.  Bottle or urn must be empty, and liquid available.  (Vase is nasty.) */

int fill() {
	if(OBJ == VASE) goto L9222;
	if(OBJ == URN) goto L9224;
	if(OBJ != 0 && OBJ != BOTTLE) return(2011);
	if(OBJ == 0 && !HERE(BOTTLE)) return(8000);
	game.spk=107;
	if(LIQLOC(LOC) == 0)game.spk=106;
	if(HERE(URN) && PROP[URN] != 0)game.spk=214;
	if(LIQ(0) != 0)game.spk=105;
	if(game.spk != 107) return(2011);
	PROP[BOTTLE]=MOD(COND[LOC],4)/2*2;
	K=LIQ(0);
	if(TOTING(BOTTLE))PLACE[K]= -1;
	if(K == OIL)game.spk=108;
	 return(2011);

L9222:	game.spk=29;
	if(LIQLOC(LOC) == 0)game.spk=144;
	if(LIQLOC(LOC) == 0 || !TOTING(VASE)) return(2011);
	RSPEAK(145);
	PROP[VASE]=2;
	FIXED[VASE]= -1;
	 return(discard(true));

L9224:	game.spk=213;
	if(PROP[URN] != 0) return(2011);
	game.spk=144;
	K=LIQ(0);
	if(K == 0 || !HERE(BOTTLE)) return(2011);
	PLACE[K]=0;
	PROP[BOTTLE]=1;
	if(K == OIL)PROP[URN]=1;
	game.spk=211+PROP[URN];
	 return(2011);
}
