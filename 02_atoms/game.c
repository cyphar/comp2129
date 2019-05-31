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

/*
 * == WARNING ==
 *
 * I've been informed that some current COMP2129 students have been
 * plagiarising the code from this project, and removing the copyright
 * statement to try to hide their plagiarism.
 *
 * These attempts have obviously failed, since you're reading this warning.
 *
 * Aside from being against the University's policies on academic honesty
 * (which can lead to you being severely penalised), it's also outright
 * copyright infringement since the GPL mandates that a full copy of the
 * license and copyright information be included in copies of the work. Not to
 * mention that it's also completely unethical.
 */

/*
 * This is the actual implementation of the core game logic. commands.c is just
 * the shell-like front-end to the API exposed here.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "atoms.h"

/* The list of player colours, in the order given by the spec. */
const char *PLAYER_COLOURS[PLAYERS_MAX] = {
	"Red",
	"Green",
	"Purple",
	"Blue",
	"Yellow",
	"White",
};

/* Create a new game instance. */
struct game_t *game_alloc(void)
{
	struct game_t *game = malloc(sizeof(struct game_t));
	if (!game)
		return NULL;

	/* Zero out memory and initialise. */
	memset(game, 0, sizeof(struct game_t));
	game->state = BLANK;
	return game;
}

/* Free all of the memory used by a game instance. */
void game_free(struct game_t *game)
{
	if (game) {
		free(game->grid);
		free(game->players);
		free(game->moves);
	}
	free(game);
}

/* Initialises a game instance to be a "new game" state. */
int game_init(struct game_t *game, int players, int width, int height)
{
	if (!game)
		return -ERROR_INTERNAL;
	if (game->state != BLANK)
		/*
		 * Re-initialising a game is required to fail by the spec, but is not a
		 * limitation of this API.
		 */
		return -ERROR_INVALID_COMMAND;

	if (players < PLAYERS_MIN || players > PLAYERS_MAX)
		return -ERROR_CANNOT_START;
	if (width * height < players)
		return -ERROR_CANNOT_START;

	/* Allocate players. */
	game->num_players = players;
	game->players = malloc(game->num_players * sizeof(struct player_t));
	if (!game->players)
		goto err;
	for (size_t i = 0; i < game->num_players; i++) {
		game->players[i] = (struct player_t) {
			.idx = i,
			.colour = PLAYER_COLOURS[i],
		};
	}
	game->current_player = 0;

	/* Allocate grid. */
	game->width = width;
	game->height = height;
	game->grid = malloc(game->width * game->height * sizeof(struct cell_t));
	if (!game->grid)
		goto err_free_players;
	memset(game->grid, 0, game->width * game->height * sizeof(struct cell_t));

	/* Start with no moves. */
	game->moves_length = 0;
	game->moves = NULL;

	/* New games don't need PLAYFROM. */
	game->state = RUNNING;
	return 0;

err_free_players:
	free(game->players);
err:
	return -ERROR_INTERNAL;
}

const char *game_current_colour(struct game_t *game)
{
	return PLAYER_COLOURS[game->current_player];
}

/*
 * Fills the array pointed to by @counts with the number of cells owned by the
 * player with that index. The length of @counts should be at least
 * @game->num_players.
 */
void game_cell_count(struct game_t *game, int *counts)
{
	memset(counts, 0, game->num_players * sizeof(int));

	for (size_t i = 0; i < game->width * game->height; i++) {
		struct cell_t cell = game->grid[i];
		if (cell.owner)
			counts[cell.owner->idx]++;
	}

	/*
	 * If we're past move k (every player has had a chance to make a move) then
	 * players can lose (any player with a count of 0 has lost).
	 */
	if (game->moves_length > game->num_players) {
		for (size_t i = 0; i < game->num_players; i++) {
			if (!counts[game->players[i].idx])
				counts[game->players[i].idx] = -1;
		}
	}
}

/*
 * game_active_players returns the number of player that are still playing the
 * game. Returns < 0 if an error occurred.
 */
static int game_active_players(struct game_t *game)
{
	int *counts = malloc(game->num_players * sizeof(int));
	if (!counts)
		return -ERROR_INTERNAL;
	game_cell_count(game, counts);

	int active = 0;
	for (size_t i = 0; i < game->num_players; i++)
		if (counts[i] != -1)
			active++;

	free(counts);
	return active;
}

/* game_display graphically displays the current state of the game grid. */
void game_display(struct game_t *game, FILE *out)
{
	/* Header. */
	fputs("+", out);
	for (size_t x = 0; x < 3*game->width - 1; x++)
		fputs("-", out);
	fputs("+\n", out);

	for (size_t y = 0; y < game->height; y++) {
		for (size_t x = 0; x < game->width; x++) {
			struct cell_t cell = game->grid[y*game->width + x];
			if (cell.owner)
				fprintf(out, "|%c%d", cell.owner->colour[0], cell.atoms);
			else
				fputs("|  ", out);
		}
		fputs("|\n", out);
	}

	/* Footer. */
	fputs("+", out);
	for (size_t x = 0; x < 3*game->width - 1; x++)
		fputs("-", out);
	fputs("+\n", out);
}

/* game_is_move_inside returns whether the given move is inside the grid. */
static bool game_is_move_inside(struct game_t *game, struct move_t move)
{
	return (move.x >= 0 && move.x < (int) game->width) &&
	       (move.y >= 0 && move.y < (int) game->height);
}

/* game_get_limit gets the atom limit for a particular cell. */
static int game_get_limit(struct game_t *game, struct move_t move)
{
	/* Default limit is 4, every edge you are touching reduces it by one. */
	int limit = 4;

	if (move.x == 0 || move.x == (int) game->width - 1)
		limit--;
	if (move.y == 0 || move.y == (int) game->height - 1)
		limit--;

	return limit;
}

/*
 * game_place_cascade (forcefully) places an atom in the given location and
 * then handles the recursive resolution of "explosion" cases where atoms in
 * cells need to bleed into other cells. This is implemented separately to
 * game_do_move to not count explosions as moves. The cascade is handled
 * depth-first, clockwise (from the top) and starts from the atom placement.
 */
static int game_place_cascade(struct game_t *game, struct player_t *player, struct move_t move)
{
	int err = -ERROR_INTERNAL;
	size_t idx = game->width * move.y + move.x;
	if (idx > game->width * game->height)
		return -ERROR_INTERNAL;
	if (move.x < 0 || move.x >= (int) game->width)
		return -ERROR_INTERNAL;
	if (move.y < 0 || move.y >= (int) game->height)
		return -ERROR_INTERNAL;

	/* Update owner and count. */
	game->grid[idx].owner = player;
	game->grid[idx].atoms++;

	/*
	 * Before we do anything recursive, check whether the player has won. This is to avoid */
	int active = game_active_players(game);
	if (active < 0)
		return active;
	else if (active == 1)
		/* We've won. */
		return -ERROR_QUIT;

	/*
	 * If we're not over the limit for the number of atoms, there's nothing
	 * left to do.
	 */
	if (game->grid[idx].atoms < game_get_limit(game, move))
		return 0;

	/*
	 * Set atom count to zero first. If we decrement it each time we'll run
	 * into fun issues with cases where a cell will cause us to recompute our
	 * own expansion.
	 */
	game->grid[idx].owner = NULL;
	game->grid[idx].atoms = 0;

	/* Expand in the order provided by the spec (up, right, down, left). */
	struct move_t moves[] = {
		{.x = move.x,     .y = move.y - 1},
		{.x = move.x + 1, .y = move.y},
		{.x = move.x,     .y = move.y + 1},
		{.x = move.x - 1, .y = move.y},
	};

	for (size_t i = 0; i < ARRAY_LENGTH(moves); i++) {
		struct move_t new_move = moves[i];

		if (!game_is_move_inside(game, new_move))
			continue;

		err = game_place_cascade(game, player, new_move);
		if (err < 0)
			return err;
	}

	return 0;
}

/* game_next_player advances to the next player still playing the game. */
static int game_next_player(struct game_t *game)
{
	if (game_active_players(game) <= 1)
		return -ERROR_QUIT;

	int *counts = malloc(game->num_players * sizeof(int));
	if (!counts)
		return -ERROR_INTERNAL;
	game_cell_count(game, counts);

	/* Keep advancing until we hit a player that's still playing. */
	do {
		game->current_player = (game->current_player + 1) % game->num_players;
	} while (counts[game->current_player] < 0);

	free(counts);
	return 0;
}

/*
 * game_do_move is the front-end to adding moves to the move set (that follow
 * the rules of the game). The grid is mutated as well as the @current_player.
 */
int game_do_move(struct game_t *game, struct move_t move)
{
	size_t idx = game->width * move.y + move.x;
	if (idx > game->width * game->height)
		return -ERROR_INTERNAL;

	/* Check ownership. */
	struct player_t *current = &game->players[game->current_player];
	struct player_t *owner = game->grid[idx].owner;
	if (owner && owner->idx != current->idx)
		return -ERROR_CANNOT_PLACE;

	/* Place and cascade. */
	int err = game_place_cascade(game, current, move);
	if (err < 0)
		return err;

	/* Add it to the list of moves. */
	game->moves = realloc(game->moves, ++game->moves_length * sizeof(struct move_t));
	if (!game->moves)
		return -ERROR_INTERNAL;
	game->moves[game->moves_length - 1] = move;

	/* Update the current player. */
	return game_next_player(game);
}

/*
 * game_apply_moves swaps out a game's move history with an alternative
 * history, recomputing the board and game state. This is used both by UNDO and
 * PLAYFROM. new_moves can be a pointer to game->moves or entirely separate.
 */
int game_apply_moves(struct game_t *game, struct move_t *new_moves, size_t length)
{
	int err = -ERROR_INTERNAL;

	/* Reset everything. */
	game->current_player = 0;
	game->moves_length = 0;
	memset(game->grid, 0, game->width * game->height * sizeof(struct cell_t));

	/*
	 * Create a copy of new_moves, since it might actually be pointing inside
	 * the old game->moves.
	 */
	struct move_t *moves_cpy = malloc(length * sizeof(struct move_t));
	memcpy(moves_cpy, new_moves, length * sizeof(struct move_t));

	/* Free game->moves now that we have a copy, to be safe. */
	free(game->moves);
	game->moves = NULL;

	/* If we're applying a moveset that's empty we don't have to do anything else. */
	if (length <= 0)
		goto out;

	/* Apply all of the moves. */
	for (size_t i = 0; i < length; i++) {
		err = game_do_move(game, moves_cpy[i]);
		if (err < 0)
			goto err;
	}

out:
	err = 0;
err:
	/* TODO: This leaves the game in an inconsistent state. */
	free(moves_cpy);
	return err;
}

/* save_header represents the header of savefiles. */
struct save_header_t {
	uint8_t width;
	uint8_t height;
	uint8_t no_players;
};

/* save_move_t represents each one of the entries in the savefile. */
union save_move_t {
	uint32_t raw;
	struct {
		uint8_t x, y;
		uint16_t unused;
	} parsed;
};

/*
 * game_savefile_load loads game data and recomputes the game state from the
 * given file descriptor. Note that the on-disk format for save files is
 * endianness dependent (so don't try to play SystemZ save files).
 */
int game_savefile_load(struct game_t *game, int fd)
{
	if (!game || fd < 0)
		return -ERROR_INTERNAL;
	if (game->moves != NULL)
		/* We could handle this, but it's not required by the spec. */
		return -ERROR_INTERNAL;

	/* Read the header. */
	/* TODO: This is probably not safe wrt struct packing. */
	struct save_header_t header = {0};
	size_t n = read(fd, &header, sizeof(header));
	if (n < sizeof(header))
		return -ERROR_INTERNAL;

	/* Create a new game with the parameters provided in the save file. */
	int err = game_init(game, header.no_players, header.width, header.height);
	if (err < 0)
		return err;
	/* Undo the state change to be safe. */
	game->state = BLANK;

	/* Rest of the data is move data. */
	size_t moves_length = 0;
	struct move_t *moves = NULL;
	while (true) {
		/* TODO: This is probably not safe wrt struct packing. */
		union save_move_t move = {0};
		size_t n = read(fd, &move.raw, sizeof(move.raw));
		if (n == 0)
			break;
		if (n < sizeof(move))
			goto err_free_moves;

		/* Purely a sanity check to avoid reading garbage data. */
		if (move.parsed.unused != 0)
			goto err_free_moves;

		/* Add to move list. */
		moves = realloc(moves, ++moves_length * sizeof(*moves));
		moves[moves_length - 1] = (struct move_t) {
			.x = move.parsed.x,
			.y = move.parsed.y,
		};
	}

	/* Commit move changes and change state. */
	game->moves = moves;
	game->moves_length = moves_length;
	game->state = LOADED;
	return 0;

err_free_moves:
	free(moves);
	return err;
}

/*
 * game_savefile_save writes the current game state to the given file
 * descriptor. Note that the on-disk format for save files is endianness
 * dependent (so don't try to play SystemZ save files).
 */
int game_savefile_save(struct game_t *game, int fd)
{
	if (!game || fd < 0)
		return -ERROR_INTERNAL;

	/* Create header. */
	struct save_header_t hdr = {
		.width      = game->width & 0xff,
		.height     = game->height & 0xff,
		.no_players = game->num_players & 0xff,
	};

	size_t n = write(fd, &hdr, sizeof(hdr));
	if (n != sizeof(hdr))
		return -ERROR_INTERNAL;

	/*
	 * Create moves entries. In a nice world, we could just write game->moves
	 * without a serialization step, but doing that isn't a good idea (it binds
	 * the on-disk format to the internal format).
	 */

	for (size_t i = 0; i < game->moves_length; i++) {
		union save_move_t move = {
			.parsed = {
				.x = game->moves[i].x & 0xff,
				.y = game->moves[i].y & 0xff,
				.unused = 0,
			},
		};

		size_t n = write(fd, &move.raw, sizeof(move.raw));
		if (n != sizeof(move.raw))
			return -ERROR_INTERNAL;
	}

	/* Ensure things were written to disk. */
	fsync(fd);
	return 0;
}
