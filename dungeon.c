/*
 * The dungeon compiler. Turns adventure.text into a set of C initializers
 * defining (mostly) invariant state.  A couple of slots are messed with
 * at runtime.
 */
#include "common.h"

#define LINESIZE 100
#define RTXSIZ 277
#define CLSMAX 12
#define LINSIZ 12600
#define TRNSIZ 5
#define TABSIZ 330
#define VRBSIZ 35
#define TRVSIZ 885
#define TOKLEN 5
#define HINTLEN 5

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

/* hard limit, will be propagated to database.h */
#define NOBJECTS	100

// Global variables for use in functions below that can gradually disappear as code is cleaned up
static long LNLENG;
static long LNPOSN;
static char INLINE[LINESIZE+1];
static long OLDLOC;
static long MAXDIE;

// Storage for what comes out of the database
long LINUSE;
long TRVS;
long CLSSES;
long TRNVLS;
long TABNDX;
long HNTMAX;
long PTEXT[NOBJECTS+1];
long RTEXT[RTXSIZ + 1];
long CTEXT[CLSMAX + 1];
long OBJSND[NOBJECTS+1];
long OBJTXT[NOBJECTS+1];
long STEXT[LOCSIZ + 1];
long LTEXT[LOCSIZ + 1];
long COND[LOCSIZ + 1];
long KEY[LOCSIZ + 1];
long LOCSND[LOCSIZ + 1];
long LINES[LINSIZ + 1];
long CVAL[CLSMAX + 1];
long TTEXT[TRNSIZ + 1];
long TRNVAL[TRNSIZ + 1];
long TRAVEL[TRVSIZ + 1];
long KTAB[TABSIZ + 1];
long ATAB[TABSIZ + 1];
long PLAC[NOBJECTS+1];
long FIXD[NOBJECTS+1];
long ACTSPK[VRBSIZ + 1];
long HINTS[HNTSIZ + 1][HINTLEN];

bool is_set(long, long);
long GETTXT(long, long, long);
void BUG(long);
void MAPLIN(FILE*);
long GETNUM(FILE*);
int read_database(FILE*);
void read_messages(FILE*, long);
void read_section3_stuff(FILE*);
void read_vocabulary(FILE*);
void read_initial_locations(FILE*);
void read_action_verb_message_nr(FILE*);
void read_conditions(FILE*);
void read_hints(FILE*);
void read_sound_text(FILE*);
void write_0d(FILE*, FILE*, long, char*);
void write_1d(FILE*, FILE*, long[], long, char*);
void write_hints(FILE*, FILE*, long[][HINTLEN], long, long, char*);
void write_files(FILE*, FILE*);

bool is_set(long var, long position)
{
  long mask = 1l << position;
  bool result = (var & mask) == mask;
  return(result);
}

long GETTXT(long SKIP,long ONEWRD, long UPPER) {
/*  Take characters from an input line and pack them into 30-bit words.
 *  Skip says to skip leading blanks.  ONEWRD says stop if we come to a
 *  blank.  UPPER says to map all letters to uppercase.  If we reach the
 *  end of the line, the word is filled up with blanks (which encode as 0's).
 *  If we're already at end of line when GETTXT is called, we return -1. */

  long TEXT;
  static long SPLITTING = -1;

  if(LNPOSN != SPLITTING)
    SPLITTING = -1;
  TEXT= -1;
  while (true) {
    if(LNPOSN > LNLENG)
      return(TEXT);
    if((!SKIP) || INLINE[LNPOSN] != 0)
      break;
    LNPOSN=LNPOSN+1;
  }

  TEXT=0;
  for (int I=1; I<=TOKLEN; I++) {
    TEXT=TEXT*64;
    if(LNPOSN > LNLENG || (ONEWRD && INLINE[LNPOSN] == 0))
      continue;
    char current=INLINE[LNPOSN];
    if(current < 63) {
      SPLITTING = -1;
      if(UPPER && current >= 37)
        current=current-26;
      TEXT=TEXT+current;
      LNPOSN=LNPOSN+1;
      continue;
    }
    if(SPLITTING != LNPOSN) {
      TEXT=TEXT+63;
      SPLITTING = LNPOSN;
      continue;
    }

    TEXT=TEXT+current-63;
    SPLITTING = -1;
    LNPOSN=LNPOSN+1;
  }

  return(TEXT);
}

void BUG(long NUM) {

/*  The following conditions are currently considered fatal bugs.  Numbers < 20
 *  are detected while reading the database; the others occur at "run time".
 *	0	Message line > 70 characters
 *	1	Null line in message
 *	2	Too many words of messages
 *	3	Too many travel options
 *	4	Too many vocabulary words
 *	5	Required vocabulary word not found
 *	6	Too many RTEXT messages
 *	7	Too many hints
 *	8	Location has cond bit being set twice
 *	9	Invalid section number in database
 *	10	Too many locations
 *	11	Too many class or turn messages
 *	20	Special travel (500>L>300) exceeds goto list
 *	21	Ran off end of vocabulary table
 *	22	Vocabulary type (N/1000) not between 0 and 3
 *	23	Intransitive action verb exceeds goto list
 *	24	Transitive action verb exceeds goto list
 *	25	Conditional travel entry with no alternative
 *	26	Location has no travel entries
 *	27	Hint number exceeds goto list
 *	28	Invalid month returned by date function
 *	29	Too many parameters given to SETPRM */

  fprintf(stderr, "Fatal error %ld.  See source code for interpretation.\n", NUM);
  exit(EXIT_FAILURE);
}

void MAPLIN(FILE *OPENED) {
/*  Read a line of input, from the specified input source,
 *  translate the chars to integers in the range 0-126 and store
 *  them in the common array "INLINE".  Integer values are as follows:
 *     0   = space [ASCII CODE 40 octal, 32 decimal]
 *    1-2  = !" [ASCII 41-42 octal, 33-34 decimal]
 *    3-10 = '()*+,-. [ASCII 47-56 octal, 39-46 decimal]
 *   11-36 = upper-case letters
 *   37-62 = lower-case letters
 *    63   = percent (%) [ASCII 45 octal, 37 decimal]
 *   64-73 = digits, 0 through 9
 *  Remaining characters can be translated any way that is convenient;
 *  The "TYPE" routine below is used to map them back to characters when
 *  necessary.  The above mappings are required so that certain special
 *  characters are known to fit in 6 bits and/or can be easily spotted.
 *  Array elements beyond the end of the line should be filled with 0,
 *  and LNLENG should be set to the index of the last character.
 *
 *  If the data file uses a character other than space (e.g., tab) to
 *  separate numbers, that character should also translate to 0.
 *
 *  This procedure may use the map1,map2 arrays to maintain static data for
 *  the mapping.  MAP2(1) is set to 0 when the program starts
 *  and is not changed thereafter unless the routines on this page choose
 *  to do so. */

  do {
    fgets(INLINE + 1, sizeof(INLINE) - 1, OPENED);
  }
  while (!feof(OPENED) && INLINE[1] == '#');

  LNLENG = 0;
  for (size_t i = 1; i <= sizeof(INLINE) && INLINE[i] != 0; ++i)
    {
      char val = INLINE[i];
      INLINE[i] = ascii_to_advent[(unsigned)val];
      if (INLINE[i] != 0)
	LNLENG = i;
    }
  LNPOSN = 1;
}

long GETNUM(FILE *source) {
/*  Obtain the next integer from an input line.  If K>0, we first read a
 *  new input line from a file; if K<0, we read a line from the keyboard;
 *  if K=0 we use a line that has already been read (and perhaps partially
 *  scanned).  If we're at the end of the line or encounter an illegal
 *  character (not a digit, hyphen, or blank), we return 0. */

  long DIGIT, GETNUM, SIGN;

  if(source != NULL) MAPLIN(source);
  GETNUM = 0;

  while (INLINE[LNPOSN] == 0) {
    if (LNPOSN > LNLENG) return(GETNUM);
    ++LNPOSN;
  }

  if(INLINE[LNPOSN] != 9)
    {
      SIGN=1;
    }
  else
    {
      SIGN= -1;
      LNPOSN=LNPOSN+1;
    }
  while (!(LNPOSN > LNLENG || INLINE[LNPOSN] == 0))
    {
      DIGIT=INLINE[LNPOSN]-64;
      if(DIGIT < 0 || DIGIT > 9)
        {
          GETNUM=0;
          break;
        }
      GETNUM=GETNUM*10+DIGIT;
      LNPOSN=LNPOSN+1;
    }

  GETNUM=GETNUM*SIGN;
  LNPOSN=LNPOSN+1;
  return(GETNUM);
}

int read_database(FILE* database) {

/*  Clear out the various text-pointer arrays.  All text is stored in array
 *  lines; each line is preceded by a word pointing to the next pointer (i.e.
 *  the word following the end of the line).  The pointer is negative if this is
 *  first line of a message.  The text-pointer arrays contain indices of
 *  pointer-words in lines.  STEXT(N) is short description of location N.
 *  LTEXT(N) is long description.  PTEXT(N) points to message for game.prop(N)=0.
 *  Successive prop messages are found by chasing pointers.  RTEXT contains
 *  section 6's stuff.  CTEXT(N) points to a player-class message.  TTEXT is for
 *  section 14.  We also clear COND (see description of section 9 for details). */

  for (int I=1; I<=NOBJECTS; I++) {
    PTEXT[I] = 0;
    OBJSND[I] = 0;
    OBJTXT[I] = 0;
  }
  for (int I=1; I<=RTXSIZ; I++) {
    RTEXT[I] = 0;
  }
  for (int I=1; I<=CLSMAX; I++) {
    CTEXT[I] = 0;
  }
  for (int I=1; I<=LOCSIZ; I++) {
    STEXT[I] = 0;
    LTEXT[I] = 0;
    COND[I] = 0;
    KEY[I] = 0;
    LOCSND[I] = 0;
  }

  LINUSE = 1;
  TRVS = 1;
  CLSSES = 0;
  TRNVLS = 0;

/*  Start new data section.  Sect is the section number. */

  while(true)
    {
      long sect=GETNUM(database);
      OLDLOC= -1;
      switch (sect)
        {
          case 0: return(0);
          case 1: read_messages(database, sect); break;
          case 2: read_messages(database, sect); break;
          case 3: read_section3_stuff(database); break;
          case 4: read_vocabulary(database); break;
          case 5: read_messages(database, sect); break;
          case 6: read_messages(database, sect); break;
          case 7: read_initial_locations(database); break;
          case 8: read_action_verb_message_nr(database); break;
          case 9: read_conditions(database); break;
          case 10: read_messages(database, sect); break;
          case 11: read_hints(database); break;
          case 12: break;
          case 13: read_sound_text(database); break;
          case 14: read_messages(database, sect); break;
          default: BUG(9);
        }
    }
}


/*  Sections 1, 2, 5, 6, 10, 14.  Read messages and set up pointers. */
void read_messages(FILE* database, long sect)
  {
    long KK=LINUSE;
    while(true)
      {
        long loc;
        LINUSE=KK;
        loc=GETNUM(database);
        if(LNLENG >= LNPOSN+70)BUG(0);
        if(loc == -1) return;
        if(LNLENG < LNPOSN)BUG(1);
        do {
            KK=KK+1;
            if(KK >= LINSIZ)BUG(2);
            LINES[KK]=GETTXT(false,false,false);
          }
        while(LINES[KK] != -1);
        LINES[LINUSE]=KK;
        if(loc == OLDLOC) continue;
        OLDLOC=loc;
        LINES[LINUSE]= -KK;
        if(sect == 14)
          {
            TRNVLS=TRNVLS+1;
            if(TRNVLS > TRNSIZ)BUG(11);
            TTEXT[TRNVLS]=LINUSE;
            TRNVAL[TRNVLS]=loc;
            continue;
          }
        if(sect == 10)
          {
            CLSSES=CLSSES+1;
            if(CLSSES > CLSMAX)BUG(11);
            CTEXT[CLSSES]=LINUSE;
            CVAL[CLSSES]=loc;
            continue;
          }
        if(sect == 6)
          {
            if(loc > RTXSIZ)BUG(6);
            RTEXT[loc]=LINUSE;
            continue;
          }
        if(sect == 5)
          {
            if(loc > 0 && loc <= NOBJECTS)PTEXT[loc]=LINUSE;
            continue;
          }
        if(loc > LOCSIZ)BUG(10);
        if(sect == 1)
          {
            LTEXT[loc]=LINUSE;
            continue;
          }

        STEXT[loc]=LINUSE;
      }
  }


/*  The stuff for section 3 is encoded here.  Each "from-location" gets a
 *  contiguous section of the "TRAVEL" array.  Each entry in travel is
 *  newloc*1000 + KEYWORD (from section 4, motion verbs), and is negated if
 *  this is the last entry for this location.  KEY(N) is the index in travel
 *  of the first option at location N. */
void read_section3_stuff(FILE* database)
  {
    long loc;
    while((loc=GETNUM(database)) != -1)
      {
        long newloc=GETNUM(NULL);
        long L;
        if(KEY[loc] == 0)
          {
            KEY[loc]=TRVS;
          }
        else
          {
            TRAVEL[TRVS-1]= -TRAVEL[TRVS-1];
          }
        while((L=GETNUM(NULL)) != 0)
          {
            TRAVEL[TRVS]=newloc*1000+L;
            TRVS=TRVS+1;
            if(TRVS == TRVSIZ)BUG(3);
          }
        TRAVEL[TRVS-1]= -TRAVEL[TRVS-1];
      }
  }


/*  Here we read in the vocabulary.  KTAB(N) is the word number, ATAB(N) is
 *  the corresponding word.  The -1 at the end of section 4 is left in KTAB
 *  as an end-marker. */
void read_vocabulary(FILE* database)
  {
    for (TABNDX=1; TABNDX<=TABSIZ; TABNDX++)
      {
        KTAB[TABNDX]=GETNUM(database);
        if(KTAB[TABNDX] == -1) return;
        ATAB[TABNDX]=GETTXT(true,true,true);
      } /* end loop */
    BUG(4);
  }


/*  Read in the initial locations for each object.  Also the immovability info.
 *  plac contains initial locations of objects.  FIXD is -1 for immovable
 *  objects (including the snake), or = second loc for two-placed objects. */
void read_initial_locations(FILE* database)
  {
    long OBJ;
    while((OBJ=GETNUM(database)) != -1)
      {
        PLAC[OBJ]=GETNUM(NULL);
        FIXD[OBJ]=GETNUM(NULL);
      }
  }


/*  Read default message numbers for action verbs, store in ACTSPK. */
void read_action_verb_message_nr(FILE* database)
  {
    long verb;
    while((verb=GETNUM(database)) != -1)
      {
        ACTSPK[verb]=GETNUM(NULL);
      }
  }


/*  Read info about available liquids and other conditions, store in COND. */
void read_conditions(FILE* database)
  {
    long K;
    while((K=GETNUM(database)) != -1)
      {
        long loc;
        while((loc=GETNUM(NULL)) != 0)
          {
            if(is_set(COND[loc],K)) BUG(8);
            COND[loc]=COND[loc] + (1l << K);
          }
      }
  }


/*  Read data for hints. */
void read_hints(FILE* database)
  {
    long K;
    HNTMAX=0;
    while((K=GETNUM(database)) != -1)
      {
        if(K <= 0 || K > HNTSIZ)BUG(7);
        for (int I=1; I<=4; I++)
          {
            HINTS[K][I] =GETNUM(NULL);
          } /* end loop */
	HNTMAX=(HNTMAX>K ? HNTMAX : K);
      }
  }


/*  Read the sound/text info, store in OBJSND, OBJTXT, LOCSND. */
void read_sound_text(FILE* database)
  {
    long K;
    while((K=GETNUM(database)) != -1)
      {
        long KK=GETNUM(NULL);
        long I=GETNUM(NULL);
        if(I != 0)
          {
            OBJSND[K]=(KK>0 ? KK : 0);
            OBJTXT[K]=(I>0 ? I : 0);
            continue;
          }

        LOCSND[K]=KK;
      }
  }

/*  Finish constructing internal data format */

/*  Having read in the database, certain things are now constructed.
 *  game.propS are set to zero.  We finish setting up COND by checking for
 *  forced-motion travel entries.  The PLAC and FIXD arrays are used
 *  to set up game.atloc(N) as the first object at location N, and
 *  game.link(OBJ) as the next object at the same location as OBJ.
 *  (OBJ>NOBJECTS indicates that game.fixed(OBJ-NOBJECTS)=LOC; game.link(OBJ) is
 *  still the correct link to use.)  game.abbrev is zeroed; it controls
 *  whether the abbreviated description is printed.  Counts modulo 5
 *  unless "LOOK" is used. */

void write_0d(FILE* c_file, FILE* header_file, long single, char* varname)
{
  fprintf(c_file, "long %s = %ld;\n", varname, single);
  fprintf(header_file, "extern long %s;\n", varname);
}

void write_1d(FILE* c_file, FILE* header_file, long array[], long dim, char* varname)
{
  fprintf(c_file, "long %s[] = {\n", varname);
  for (int i = 0; i < dim; ++i)
    {
      if (i % 10 == 0)
	{
	  if (i > 0)
	    fprintf(c_file, "\n");
	  fprintf(c_file, "  ");
	}
      fprintf(c_file, "%ld, ", array[i]);
    }
  fprintf(c_file, "\n};\n");
  fprintf(header_file, "extern long %s[%ld];\n", varname, dim);
}

void write_hints(FILE* c_file, FILE* header_file, long matrix[][HINTLEN], long dim1, long dim2, char* varname)
{
  fprintf(c_file, "long %s[][%ld] = {\n", varname, dim2);
  for (int i = 0; i < dim1; ++i)
    {
      fprintf(c_file, "  {");
      for (int j = 0; j < dim2; ++j)
	{
	  fprintf(c_file, "%ld, ", matrix[i][j]);
	}
      fprintf(c_file, "},\n");
    }
  fprintf(c_file, "};\n");
  fprintf(header_file, "extern long %s[%ld][%ld];\n", varname, dim1, dim2);
}

void write_files(FILE* c_file, FILE* header_file)
{
  int i, MAXDIE;
  for (i=0; i<=4; i++) {
      long x = 2*i+81;
      if(RTEXT[x] != 0)
	  MAXDIE=i+1;
  }

  // preprocessor defines for the header
  fprintf(header_file, "#include \"common.h\"\n");
  fprintf(header_file, "#define RTXSIZ 277\n");
  fprintf(header_file, "#define CLSMAX 12\n");
  fprintf(header_file, "#define LINSIZ %d\n", LINSIZ);
  fprintf(header_file, "#define TRNSIZ 5\n");
  fprintf(header_file, "#define TABSIZ 330\n");
  fprintf(header_file, "#define VRBSIZ 35\n");
  fprintf(header_file, "#define HNTSIZ 20\n");
  fprintf(header_file, "#define TRVSIZ 885\n");
  fprintf(header_file, "#define TOKLEN %d\n", TOKLEN);
  fprintf(header_file, "#define HINTLEN %d\n", HINTLEN);
  fprintf(header_file, "#define MAXDIE %d\n", MAXDIE);
  fprintf(header_file, "\n");

  // include the header in the C file
  fprintf(c_file, "#include \"database.h\"\n");
  fprintf(c_file, "\n");

  // content variables
  write_0d(c_file, header_file, LINUSE, "LINUSE");
  write_0d(c_file, header_file, TRVS, "TRVS");
  write_0d(c_file, header_file, CLSSES, "CLSSES");
  write_0d(c_file, header_file, TRNVLS, "TRNVLS");
  write_0d(c_file, header_file, TABNDX, "TABNDX");
  write_0d(c_file, header_file, HNTMAX, "HNTMAX");
  write_1d(c_file, header_file, PTEXT, NOBJECTS + 1, "PTEXT");
  write_1d(c_file, header_file, RTEXT, RTXSIZ + 1, "RTEXT");
  write_1d(c_file, header_file, CTEXT, CLSMAX + 1, "CTEXT");
  write_1d(c_file, header_file, OBJSND, NOBJECTS + 1, "OBJSND");
  write_1d(c_file, header_file, OBJTXT, NOBJECTS + 1, "OBJTXT");
  write_1d(c_file, header_file, STEXT, LOCSIZ + 1, "STEXT");
  write_1d(c_file, header_file, LTEXT, LOCSIZ + 1, "LTEXT");
  write_1d(c_file, header_file, COND, LOCSIZ + 1, "COND");
  write_1d(c_file, header_file, KEY, LOCSIZ + 1, "KEY");
  write_1d(c_file, header_file, LOCSND, LOCSIZ + 1, "LOCSND");
  write_1d(c_file, header_file, LINES, LINSIZ + 1, "LINES");
  write_1d(c_file, header_file, CVAL, CLSMAX + 1, "CVAL");
  write_1d(c_file, header_file, TTEXT, TRNSIZ + 1, "TTEXT");
  write_1d(c_file, header_file, TRNVAL, TRNSIZ + 1, "TRNVAL");
  write_1d(c_file, header_file, TRAVEL, TRVSIZ + 1, "TRAVEL");
  write_1d(c_file, header_file, KTAB, TABSIZ + 1, "KTAB");
  write_1d(c_file, header_file, ATAB, TABSIZ + 1, "ATAB");
  write_1d(c_file, header_file, PLAC, NOBJECTS + 1, "PLAC");
  write_1d(c_file, header_file, FIXD, NOBJECTS + 1, "FIXD");
  write_1d(c_file, header_file, ACTSPK, VRBSIZ + 1, "ACTSPK");
  write_hints(c_file, header_file, HINTS, HNTSIZ + 1, 5, "HINTS");
}

int main()
{
  FILE* database = fopen("adventure.text", "r");
  read_database(database);
  fclose(database);

  FILE* c_file = fopen("database.c", "w");
  FILE* header_file = fopen("database.h", "w");
  write_files(c_file, header_file);
  fclose(c_file);
  fclose(header_file);

  return(EXIT_SUCCESS);
}
