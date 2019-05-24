/*
 * Initialisation
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "advent.h"
#include "calc.h"


#ifndef CALCULATOR
void load_failure(char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);    
}

FILE dungeon_file;

uint8_t get_byte()
{
    uint8_t ret;
    if (fread(&ret, 1, 1, dungeon_file) != 1)
        load_failure("Out of bytes in input");
    return ret;
}

uint16_t get_word()
{
    uint16_t ret;
    ret = get_byte() | (get_byte() << 8);
    return ret;
}

int load_dungeon(void)
{
    char id_str[32];
    long huffman_table_location, compressed_strings_location, uncompressed_strings_location,
        arbitrary_messages_location, classes_location, turn_thresholds_location,
        locations_location, objects_location, obituaries_location, hints_location,
        motions_location, actions_locations, tkey_location, travel_location;
    long size, current_item, next_item;
    int i;
    
    dungeon_file = fopen("dungeon.bin", "rb");
    if (!dungeon_file)
        load_failure("Failed to open dungeon.bin");
    fseek(dungeon_file, 0, SEEK_SET);
    fread(&id_str[0], 1, sizeof(id_str), dungeon_file);
    if (!id_str[31] && !strcmp(id_str, DATA_FILE_ID_STRING))
        load_failure("dungeon.bin does not look like a dungeon file");
    /* Get the location of various items */
    huffman_table_location = get_word();
    compressed_strings_location = get_word();
    uncompressed_strings_location = get_word();
    arbitrary_messages_location = get_word();
    classes_location = get_word();
    turn_thresholds_location = get_word();
    locations_location = get_word();
    objects_location = get_word();
    obituaries_location = get_word();
    hints_location = get_word();
    motions_location = get_word();
    actions_locations = get_word();
    tkey_location = get_word();
    travel_location = get_word();
    /* Huffman table */
    size = compressed_strings_location - huffman_table_location;
    huffman_table = malloc(size);
    fseek(dungeon_file, huffman_table_location, SEEK_SET);
    if (size != fread(huffman_table, 1, size, dungeon_file))
        load_failure("Error loading Huffman table");
    /* Compressed strings */
    compressed_strings = malloc(size_t * NCOMPSTRS);
    for (i = 0; i < NCOMPSTRS; i++)
    {
        fseek(dungeon_file, compressed_strings_location, SEEK_SET);
        compressed_strings_location += 2;
        current_item = get_word();
        if (i < NCOMPSTRS - 1)
            next_item = get_word();
        else
            next_item = uncompressed_strings_location;
        size = next_item - current_item;
        compressed_strings[i] = malloc(size);
        fseek(dungeon_file, current_item, SEEK_SET);
        if (size != fread(&compressed_string[i], 1, size, dungeon_file))
            load_failure("Error loading compressed strings");
    }
    /* Uncompressed strings */
    uncompressed_strings = malloc(size_t * NCOMPSTRS);
    for (i = 0; i < NCOMPSTRS; i++)
    {
        fseek(dungeon_file, uncompressed_strings_location, SEEK_SET);
        uncompressed_strings_location += 2;
        current_item = get_word();
        if (i < NCOMPSTRS - 1)
            next_item = get_word();
        else
            next_item = arbitrary_messages_location;
        size = next_item - current_item;
        uncompressed_strings[i] = malloc(size);
        fseek(dungeon_file, current_item, SEEK_SET);
        if (size != fread(&uncompressed_string[i], 1, size, dungeon_file))
            load_failure("Error loading uncompressed strings");
    }
    /* Arbitrary messages cross reference table */
    arbitrary_messages = malloc(sizeof(compressed_string_index_t) * NARBMSGS);
    fseek(dungeon_file, arbitrary_messages_location, SEEK_SET);
    for (i = 0; i < NARBMSGS; i++)
        arbitrary_messages[i] = get_word();
    /* Classes */
    classes = malloc(sizeof(class_t) * (NCLASSES + 1));
    fseek(dungeon_file, classes_location, SEEK_SET);
    for (i = 0; i < NCLASSES + 1; i++)
    {
        classes[i]->threshold = get_word();
        classes[i]->message = get_word();
    }
    /* Turn thresholds */
    turn_thresholds = malloc(sizeof(turn_threshold_t) * (NTHRESHOLDS + 1));
    fseek(dungeon_file, turn_thresholds_location, SEEK_SET);
    for (i = 0; i < NTHRESHOLDS + 1; i++)
    {
        turn_thresholds[i]->threshold = get_word();
        turn_thresholds[i]->point_loss = get_byte();
        turn_thresholds[i]->message = get_word();
    }
    /* Locations */
    locations = malloc(sizeof(location_t) * (NLOCATIONS + 1));
    fseek(dungeon_file, locations_location, SEEK_SET);
    for (i = 0; i < NLOCATIONS + 1; i++)
    {
        locations[i]->description.small = get_word();
        locations[i]->description.big = get_word();
        locations[i]->sound = get_byte();
        locations[i]->loud = get_byte();
    }
    
    
    
    
    fseek(dungeon_file, 0, SEEK_END);
    long size = ftell(dungeon_file);
    fseek(dungeon_file, 0, SEEK_SET);
    
    
}
#endif



struct settings_t settings = {
    .logfp = NULL,
    .oldstyle = false,
    .prompt = true
};

/* Inline initializers moved to function below for ANSI Cness */
struct game_t game;

int initialise(void)
{
    int i, k, treasure;
    int seedval;
#ifndef CALCULATOR
    if (settings.oldstyle)
        printf("Initialising...\n");
    load_dungeon();
#endif
    
    
    game.dloc[1] = LOC_KINGHALL;
    game.dloc[2] = LOC_WESTBANK;
    game.dloc[3] = LOC_Y2;
    game.dloc[4] = LOC_ALIKE3;
    game.dloc[5] = LOC_COMPLEX;

    /*  Sixth dwarf is special (the pirate).  He always starts at his
     *  chest's eventual location inside the maze. This loc is saved
     *  in chloc for ref. The dead end in the other maze has its
     *  loc stored in chloc2. */
    game.dloc[6] = LOC_DEADEND12;
    game.chloc   = LOC_DEADEND12;
    game.chloc2  = LOC_DEADEND13;
    game.abbnum  = 5;
    game.clock1  = WARNTIME;
    game.clock2  = FLASHTIME;
    game.newloc  = LOC_START;
    game.loc     = LOC_START;
    game.limit   = GAMELIMIT;
    game.foobar  = WORD_EMPTY;

    srand(time(NULL));
    seedval = (int)rand();
    set_seed(seedval);

    for (i = 1; i <= NOBJECTS; i++) {
        game.place[i] = LOC_NOWHERE;
    }

    for (i = 1; i <= NLOCATIONS; i++) {
        if (!(get_location(i)->description.big == 0 ||
              get_tkey(i) == 0)) {
            k = get_tkey(i);
            if (T_TERMINATE(get_travelop(k)))
                conditions[i] |= (1 << COND_FORCED);
        }
    }

    /*  Set up the game.atloc and game.link arrays.
     *  We'll use the DROP subroutine, which prefaces new objects on the
     *  lists.  Since we want things in the other order, we'll run the
     *  loop backwards.  If the object is in two locs, we drop it twice.
     *  Also, since two-placed objects are typically best described
     *  last, we'll drop them first. */
    for (i = NOBJECTS; i >= 1; i--) {
        if (get_object(i)->fixd > 0) {
            drop(i + NOBJECTS, get_object(i)->fixd);
            drop(i, get_object(i)->plac);
        }
    }

    for (i = 1; i <= NOBJECTS; i++) {
        k = NOBJECTS + 1 - i;
        game.fixed[k] = get_object(k)->fixd;
        if (get_object(k)->plac != 0 && get_object(k)->fixd <= 0)
            drop(k, get_object(k)->plac);
    }

    /*  Treasure props are initially -1, and are set to 0 the first time
     *  they are described.  game.tally keeps track of how many are
     *  not yet found, so we know when to close the cave. */
    for (treasure = 1; treasure <= NOBJECTS; treasure++) {
        if (get_object(treasure)->is_treasure) {
            if (get_object(treasure)->inventory != 0)
                game.prop[treasure] = STATE_NOTFOUND;
            game.tally = game.tally - game.prop[treasure];
        }
    }
    game.conds = setbit(11);

    return seedval;
}
