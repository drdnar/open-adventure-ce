#include "dungeon.h"
#include "calc.h"
#include "advent.h"

#ifndef CALCULATOR
/* For non-calculator platforms, the data will not be moved after loading and
 * aligning.  Do not attempt to reload the data. */
bool have_data_file = 0;
#endif


/* Loads (or reloads) the data file.
 * Returns zero on success, non-zero on failure.
 */
uint8_t load_data_file()
{
#ifdef CALCULATOR
    /* For the eZ80, the data file will be mapped directly into the address
     * space.  The eZ80 fully supports unaligned accesses, so the getter
     * routines can just return a pointer to the raw data.  We'll look up the
     * data file address and then cache real pointers to our data sets. */
    return 1;
#else
    if (have_data_file)
        return 0;
    /* For other platforms, we will assume nothing about alignment requirements.
     * Therefore, we will have to "unwind" the dungeon file data structures into
     * fully-aligned dynamically allocated memory. */
     return 1;
#endif
}


