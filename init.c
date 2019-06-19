/*
 * Initialisation
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#ifndef CALCULATOR
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#ifndef CALCULATOR
#include <time.h>
#endif
#include <string.h>

#include "advent.h"
#include "dungeon.h"
#ifdef CALCULATOR
#include "calc.h"
#include "style.h"
#endif

#ifndef CALCULATOR
void load_failure(char* message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);    
}

FILE* dungeon_file;

static uint8_t get_byte()
{
    uint8_t ret;
    if (fread(&ret, 1, 1, dungeon_file) != 1)
        load_failure("Out of bytes in input");
    return ret;
}

static uint16_t get_word()
{
    uint16_t ret;
    ret = get_byte() | (get_byte() << 8);
    return ret;
}

void load_dungeon(void)
{
    char id_str[32];
    long huffman_tree_location, compressed_strings_location, uncompressed_strings_location,
        arbitrary_messages_location, classes_location, turn_thresholds_location,
        locations_location, objects_location, obituaries_location, hints_location,
        motions_location, actions_location, tkey_location, travel_location;
    long size, current_item, next_item;
    int i, j;
    
    dungeon_file = fopen("dungeon.bin", "rb");
    if (!dungeon_file)
        load_failure("Failed to open dungeon.bin");
    fseek(dungeon_file, 0, SEEK_SET);
    if (sizeof(id_str) != fread(&id_str[0], 1, sizeof(id_str), dungeon_file))
	load_failure("dungeon.bin got truncated?");
    if (id_str[31] || strcmp(id_str, DATA_FILE_ID_STRING))
        load_failure("dungeon.bin does not look like a dungeon file");
    /* Get the location of various items */
    huffman_tree_location = get_word();
    compressed_strings_location = get_word();
    uncompressed_strings_location = get_word();
    arbitrary_messages_location = get_word();
    classes_location = get_word();
    turn_thresholds_location = get_word();
    locations_location = get_word();
    objects_location = get_word();
    obituaries_location = get_word();
    hints_location = get_word();
    get_word();
    motions_location = get_word();
    actions_location = get_word();
    tkey_location = get_word();
    travel_location = get_word();
    /* Huffman table */
    size = compressed_strings_location - huffman_tree_location;
    huffman_tree = malloc(size);
    fseek(dungeon_file, huffman_tree_location, SEEK_SET);
    if (size != fread(huffman_tree, 1, size, dungeon_file))
        load_failure("Error loading Huffman table");
    /* Compressed strings */
    compressed_strings = malloc(sizeof(intptr_t) * NCOMPSTRS);
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
        if (size != fread(compressed_strings[i], 1, size, dungeon_file))
            load_failure("Error loading compressed strings");
    }
    /* Uncompressed strings */
    uncompressed_strings = malloc(sizeof(intptr_t) * NUNCOMPSTRS);
    for (i = 0; i < NUNCOMPSTRS; i++)
    {
        fseek(dungeon_file, uncompressed_strings_location, SEEK_SET);
        uncompressed_strings_location += 2;
        current_item = get_word();
        if (i < NUNCOMPSTRS - 1)
            next_item = get_word();
        else
            next_item = arbitrary_messages_location;
        size = next_item - current_item;
        uncompressed_strings[i] = malloc(size);
        fseek(dungeon_file, current_item, SEEK_SET);
        if (size != fread(uncompressed_strings[i], 1, size, dungeon_file))
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
        classes[i].threshold = get_word();
        classes[i].message = get_word();
    }
    /* Turn thresholds */
    turn_thresholds = malloc(sizeof(turn_threshold_t) * (NTHRESHOLDS));
    fseek(dungeon_file, turn_thresholds_location, SEEK_SET);
    for (i = 0; i < NTHRESHOLDS; i++)
    {
        turn_thresholds[i].threshold = get_word();
        turn_thresholds[i].point_loss = get_byte();
        turn_thresholds[i].message = get_word();
    }
    /* Locations */
    locations = malloc(sizeof(location_t) * (NLOCATIONS + 1));
    fseek(dungeon_file, locations_location, SEEK_SET);
    for (i = 0; i < NLOCATIONS + 1; i++)
    {
        locations[i].description.small = get_word();
        locations[i].description.big = get_word();
        locations[i].sound = get_byte();
        locations[i].loud = get_byte();
    }
    /* Objects */
    objects = malloc(sizeof(intptr_t) * (NOBJECTS + 1));
    for (i = 0; i < NOBJECTS + 1; i++)
    {
        /*printf("#%i 0x%04X -> ", i, objects_location);*/
	fseek(dungeon_file, objects_location, SEEK_SET);
        objects_location += 2;
        current_item = get_word();
        if (i < NOBJECTS)
            next_item = get_word();
        else
            next_item = obituaries_location;
        size = next_item - current_item;
        objects[i] = malloc(sizeof(object_t));
        fseek(dungeon_file, current_item, SEEK_SET);
        /*printf("0x%04X\n", current_item);*/
        objects[i]->inventory = get_word();
        objects[i]->plac = get_word();
        objects[i]->fixd = get_word();
        objects[i]->is_treasure = get_byte();
        objects[i]->descriptions_start = get_byte();
        objects[i]->sounds_start = get_byte();
        objects[i]->texts_start = get_byte();
        objects[i]->changes_start = get_byte();
        for (j = 0; j < 11; j++)
            objects[i]->strings[j] = get_word();
    }
    /* Obituaries */
    obituaries = malloc(sizeof(obituary_t) * NDEATHS);
    fseek(dungeon_file, obituaries_location, SEEK_SET);
    for (i = 0; i < NDEATHS; i++)
    {
        obituaries[i].query = get_word();
        obituaries[i].yes_response = get_word();
    }
    /* Hints */
    hints = malloc(sizeof(hint_t) * NHINTS);
    fseek(dungeon_file, hints_location, SEEK_SET);
    for (i = 0; i < NHINTS; i++)
    {
        hints[i].number = get_byte();
        hints[i].turns = get_byte();
        hints[i].penalty = get_byte();
        hints[i].question = get_word();
        hints[i].hint = get_word();
    }
    /* Motions */
    motions = malloc(sizeof(intptr_t) * NMOTIONS);
    for (i = 0; i < NMOTIONS; i++)
    {
        fseek(dungeon_file, motions_location, SEEK_SET);
        motions_location += 2;
        current_item = get_word();
        if (i < NMOTIONS)
            next_item = get_word();
        else
            next_item = actions_location;
        size = next_item - current_item;
        motions[i] = malloc(sizeof(motion_t));
        fseek(dungeon_file, current_item, SEEK_SET);
        motions[i]->words.n = get_byte();
        for (j = 0; j < 10; j++)
            motions[i]->words.strs[j] = get_word();
    }
    /* Actions */
    actions = malloc(sizeof(intptr_t) * NACTIONS);
    for (i = 0; i < NACTIONS; i++)
    {
        fseek(dungeon_file, actions_location, SEEK_SET);
        actions_location += 2;
        current_item = get_word();
        if (i < NACTIONS)
            next_item = get_word();
        else
            next_item = tkey_location;
        size = next_item - current_item;
        actions[i] = malloc(sizeof(action_t));
        fseek(dungeon_file, current_item, SEEK_SET);
        actions[i]->message = get_word();
        actions[i]->noaction = get_byte();
        actions[i]->words.n = get_byte();
        for (j = 0; j < 10; j++)
            actions[i]->words.strs[j] = get_word();
    }
    /* Keys, whatever those are */
    tkey = malloc(sizeof(uint16_t) * NKEYS);
    fseek(dungeon_file, tkey_location, SEEK_SET);
    for (i = 0; i < NKEYS; i++)
        tkey[i] = get_word();
    /* Giant travel thingy */
    travel = malloc(sizeof(travelop_t) * NTRAVEL);
    fseek(dungeon_file, travel_location, SEEK_SET);
    for (i = 0; i < NTRAVEL; i++)
    {
        travel[i].motion = get_byte();
        travel[i].condtype = get_byte();
        travel[i].condarg1 = get_byte();
        travel[i].condarg2 = get_byte();
        travel[i].desttype = get_byte();
        travel[i].destval = get_byte();
        travel[i].nodwarves = get_byte();
        travel[i].stop = get_byte();
    }
}
#else
void load_dungeon(void)
{
    char id_str[32];
    ti_var_t dungeon_file;
    dungeon_file = ti_Open("CCaveDun", "r");
    if (!dungeon_file)
        exit_fail("Failed to open dungeon appvar CCaveDun");
    dungeon = ti_GetDataPtr(dungeon_file);
    if (sizeof(id_str) != ti_Read(&id_str[0], sizeof(id_str), 1, dungeon_file))
    exit_fail("dungeon.bin got truncated?");
    if (id_str[31] || strcmp(id_str, DATA_FILE_ID_STRING))
        exit_fail("Appvar CCaveDun does not look like a dungeon file");
    /* In order to save some space, instead of the previous platform-independent
     * idiomatic C, I've written some optimized assembly.  This makes some semi-
     * unsafe assumptions, but then again, if the dungeon file is corrupted,
     * then other things are also probably corrupted and you're probably going
     * to crash sooner or later anyway.
     * 
     * This basically does the following:
     *  void* target = (void*)huffman_tree;
     *  uint16_t* offsets = (uint16_t*)(dungeon + sizeof(DATA_FILE_ID_STRING));
     *  unsigned char count;
     *  for (count = 15; count > 0; offsets++, target++, count--)
     *      *target = (void*)(dungeon + *offsets);
     * 
     * You should note that this code assumes that the dungeon variables are
     * placed in memory in a particular order.
     */
    asm("	.ref _huffman_tree");
    asm("	; A: Loop counter");
    asm("	; BC: Cached pointer to dungeon data");
    asm("	; DE: Offsets array read pointer");
    asm("	; IY: Target pointer to populate");
    asm("	ld	bc, (_dungeon)");
    asm("	ld	hl, 32 ; sizeof(DATA_FILE_ID_STRING)");
    asm("	add	hl, bc");
    asm("	ex	de, hl");
    asm("	ld	iy, _huffman_tree");
    asm("	ld	a, 15");
    asm("load_dungeon_loop:");
    asm("	sbc	hl, hl	; Previous use of ADD should keep C reset");
    asm("	ex	de, hl");
    asm("	ld	e, (hl)");
    asm("	inc	hl");
    asm("	ld	d, (hl)");
    asm("	inc	hl");
    asm("	ex	de, hl");
    asm("	add	hl, bc");
    asm("	ld	(iy), hl");
    asm("	lea	iy, iy + 3");
    asm("	dec	a");
    asm("	jr	nz, load_dungeon_loop");
    ti_Close(dungeon_file);
}
#endif


#ifndef CALCULATOR
/* This isn't allowed in C89, sadly. */
struct settings_t settings = {
    .logfp = NULL,
    .oldstyle = false,
    .prompt = true
};
#else
struct settings_t settings;
#endif

/* Inline initializers moved to function below for ANSI Cness */
struct game_t game;

int initialise(void)
{
    int i, k, treasure;
    int seedval;
    if (settings.oldstyle)
#ifndef CALCULATOR
        printf("Initialising...\n");
#else
        print("Initialising...\n");
#endif
    load_dungeon();
    
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
