#include <stdbool.h>

#define LINESIZE	100

typedef struct lcg_state
{
  unsigned long a, c, m, x;
} lcg_state;

extern long ABB[], ATAB[], ATLOC[], BLKLIN, DFLAG, DLOC[], FIXED[], HOLDNG,
		KTAB[], *LINES, LINK[], LNLENG, LNPOSN,
		PARMS[], PLACE[], PTEXT[], RTEXT[], TABSIZ;
extern signed char INLINE[LINESIZE+1], MAP1[], MAP2[];
extern FILE *logfp;
extern bool oldstyle;
extern lcg_state lcgstate;
