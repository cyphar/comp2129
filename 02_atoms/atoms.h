/*
 * Copyright (C) 2017 Aleksa Sarai <cyphar@cyphar.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined(ATOMS_H)
#define ATOMS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "errno.h"

/* Defined by the spec. */
#define PLAYERS_MIN 2
#define PLAYERS_MAX 6

/* TODO: Macro use to specify that a particular feature is not implemented. */
#define TODO_notimplemented() \
	do { fprintf(stderr, "TODO: not implemented\n"); fflush(stderr); } while (0)

#define bail(code, ...) \
	do { fprintf(stderr, __VA_ARGS__); exit(code); } while (0)

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(*(array)))

/* move_t represents a move by a particular player. */
struct move_t {
	int x, y;
	/*
	 * TODO: In order to make UNDO more efficient we could create snapshots of
	 *       the game state. But then we're wasting a bunch of space on
	 *       caching. On the plus side there's no cache invalidation concerns.
	 */
};

/* The list of player colours, in the order given by the spec. */
extern const char *PLAYER_COLOURS[PLAYERS_MAX];

/*
 * player_t represents a player. Ultimately it's just used as an elaborate
 * index in %PLAYER_COLOURS.
 */
struct player_t {
	/* TODO: Get rid of this struct. */
	size_t idx;
	const char *colour;
};

/*
 * cell_t stores the in-memory state of a particular cell. It is not directly
 * associated with a move, as they are generated from the list of move_ts.
 */
struct cell_t {
	struct player_t *owner;
	int atoms;
};

/*
 * Since loading is a two-stage process, there's three stages that commands
 * need to check against.
 */
enum game_state_t {
	BLANK,
	LOADED,
	RUNNING,
};

/* game_t stores the current in-memory state of the game. */
struct game_t {
	/* Is there a game running (bit redundant but makes code cleaner). */
	enum game_state_t state;

	/* Current board state. */
	size_t width, height;
	struct cell_t *grid; /* Stored in [<width>*y + x] */

	/* Player information. */
	size_t num_players;
	struct player_t *players;
	int current_player;

	/* The list of moves taken through the game. */
	size_t moves_length;
	struct move_t *moves;
};

/* Needs to be after the definition of game_t. */
#include "dispatch.h"

/*
 * Allocation and free routines for game_t. When a new game is allocated, the
 * game is not in a "started" state (use game_init or game_savefile_load).
 */
struct game_t *game_alloc(void);
void game_free(struct game_t *game);

/* Called when creating a new game. */
int game_init(struct game_t *game, int players, int width, int height);

/* Load and save routines. */
int game_savefile_load(struct game_t *game, int fd);
int game_savefile_save(struct game_t *game, int fd);

/* Commands. */
void game_cell_count(struct game_t *game, int *counts);
int game_apply_moves(struct game_t *game, struct move_t *new_moves, size_t length);
int game_do_move(struct game_t *game, struct move_t move);
void game_display(struct game_t *game, FILE *out);
const char *game_current_colour(struct game_t *game);

#endif /* !defined(ATOMS_H) */
