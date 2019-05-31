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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "atoms.h"
#include "dispatch.h"

int __cmd_HELP(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;
	if (game->state == LOADED)
		return -ERROR_INVALID_COMMAND;
	if (cmd->argc != 1)
		return -ERROR_INVALID_COMMAND;

	puts("");
	puts("HELP displays this help message");
	puts("QUIT quits the current game");
	puts("");
	puts("DISPLAY draws the game board in terminal");
	puts("START <number of players> <width> <height> starts the game");
	puts("PLACE <x> <y> places an atom in a grid space");
	puts("UNDO undoes the last move made");
	puts("STAT displays game statistics");
	puts("");
	puts("SAVE <filename> saves the state of the game");
	puts("LOAD <filename> loads a save file");
	puts("PLAYFROM <turn> plays from n steps into the game");
	return 0;
}

int __cmd_QUIT(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;
	if (cmd->argc != 1)
		return -ERROR_INVALID_COMMAND;

	puts("Bye!");
	return -ERROR_QUIT;
}

int __cmd_DISPLAY(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != RUNNING) {
		if (game->state == LOADED)
			return -ERROR_INVALID_COMMAND;
		return -ERROR_GAME_NOT_RUNNING;
	}
	if (cmd->argc != 1)
		return -ERROR_INVALID_COMMAND;

	puts("");
	game_display(game, stdout);
	return 0;
}

int __cmd_START(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != BLANK)
		return -ERROR_INVALID_COMMAND;
	if (cmd->argc < 4)
		return -ERROR_MISSING_ARGUMENTS;
	if (cmd->argc > 4)
		return -ERROR_EXCESS_ARGUMENTS;

	int players, width, height;
	char *endptr = NULL;

	/* START <k> <width> <height> */
	players = strtol(cmd->argv[1], &endptr, 10);
	if (endptr == cmd->argv[1] || *endptr)
		return -ERROR_INVALID_ARGS;
	if (players < 0)
		return -ERROR_INVALID_ARGS;

	width = strtol(cmd->argv[2], &endptr, 10);
	if (endptr == cmd->argv[2] || *endptr)
		return -ERROR_INVALID_ARGS;
	if (width < 0)
		return -ERROR_INVALID_ARGS;

	height = strtol(cmd->argv[3], &endptr, 10);
	if (endptr == cmd->argv[3] || *endptr)
		return -ERROR_INVALID_ARGS;
	if (height < 0)
		return -ERROR_INVALID_ARGS;

	int err = game_init(game, players, width, height);
	if (err < 0)
		return err;

	puts("Game Ready");
	printf("%s's Turn\n", game_current_colour(game));
	return 0;
}

int __cmd_PLACE(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != RUNNING) {
		if (game->state == LOADED)
			return -ERROR_INVALID_COMMAND;
		return -ERROR_GAME_NOT_RUNNING;
	}
	if (cmd->argc < 3)
		return -ERROR_MISSING_ARGUMENTS;
	if (cmd->argc > 3)
		return -ERROR_EXCESS_ARGUMENTS;

	char *endptr;
	struct move_t move = {0};

	move.x = strtol(cmd->argv[1], &endptr, 10);
	if (endptr == cmd->argv[1] || *endptr)
		return -ERROR_INVALID_COORDS;
	if (move.x < 0 || move.x >= (int) game->width)
		return -ERROR_INVALID_COORDS;

	move.y = strtol(cmd->argv[2], &endptr, 10);
	if (endptr == cmd->argv[2] || *endptr)
		return -ERROR_INVALID_COORDS;
	if (move.y < 0 || move.y >= (int) game->height)
		return -ERROR_INVALID_COORDS;

	int err = game_do_move(game, move);
	if (err < 0) {
		/* ERROR_QUIT means we've won. */
		if (err == -ERROR_QUIT)
			printf("%s Wins!\n", game_current_colour(game));
		return err;
	}

	printf("%s's Turn\n", game_current_colour(game));
	return 0;
}

int __cmd_UNDO(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != RUNNING) {
		if (game->state == LOADED)
			return -ERROR_INVALID_COMMAND;
		return -ERROR_GAME_NOT_RUNNING;
	}
	if (cmd->argc != 1)
		return -ERROR_INVALID_COMMAND;
	if (game->moves_length == 0)
		return -ERROR_CANNOT_UNDO;

	int err = game_apply_moves(game, game->moves, game->moves_length - 1);
	if (err < 0)
		return err;

	printf("%s's Turn\n", game_current_colour(game));
	return 0;
}

int __cmd_STAT(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != RUNNING) {
		if (game->state == LOADED)
			return -ERROR_INVALID_COMMAND;
		return -ERROR_GAME_NOT_RUNNING;
	}
	if (cmd->argc != 1)
		return -ERROR_INVALID_COMMAND;

	int *counts = malloc(game->num_players * sizeof(int));
	if (!counts)
		return -ERROR_INTERNAL;

	game_cell_count(game, counts);
	for (size_t i = 0; i < game->num_players; i++) {
		printf("Player %s:\n", game->players[i].colour);
		if (counts[i] < 0)
			printf("Lost\n");
		else
			printf("Grid Count: %d\n", counts[i]);
		if (i != game->num_players - 1)
			printf("\n");
	}

	free(counts);
	return 0;
}

int __cmd_SAVE(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != RUNNING)
		return -ERROR_INVALID_COMMAND;
	if (cmd->argc < 2)
		return -ERROR_MISSING_ARGUMENTS;
	if (cmd->argc > 2)
		return -ERROR_EXCESS_ARGUMENTS;

	char *path = cmd->argv[1];
	int fd = open(path, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) {
		if (errno == EEXIST)
			return -ERROR_FILE_EXISTS;
		return -ERROR_INTERNAL;
	}

	int err = game_savefile_save(game, fd);
	close(fd);
	if (!err)
		puts("Game Saved");
	return err;
}

int __cmd_LOAD(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != BLANK) {
		if (game->state == LOADED)
			return -ERROR_INVALID_COMMAND;
		return -ERROR_RESTART_GAME;
	}
	if (cmd->argc < 2)
		return -ERROR_MISSING_ARGUMENTS;
	if (cmd->argc > 2)
		return -ERROR_EXCESS_ARGUMENTS;

	char *path = cmd->argv[1];
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return -ERROR_CANNOT_LOAD;

	int err = game_savefile_load(game, fd);
	close(fd);
	if (!err)
		puts("Game Loaded");
	return err;
}

int __cmd_PLAYFROM(struct cmd_t *cmd, struct game_t *game)
{
	if (!cmd || !game)
		return -ERROR_INTERNAL;

	if (game->state != LOADED)
		return -ERROR_INVALID_COMMAND;
	if (cmd->argc < 2)
		return -ERROR_MISSING_ARGUMENTS;
	if (cmd->argc > 2)
		return -ERROR_EXCESS_ARGUMENTS;

	char *endptr;
	int n = strtol(cmd->argv[1], &endptr, 10);
	if (!strcmp(cmd->argv[1], "END"))
		n = game->moves_length;
	else if (endptr == cmd->argv[1] || *endptr || n < 0)
		return -ERROR_INVALID_TURN;

	if (n > (int) game->moves_length)
		n = game->moves_length;

	int err = game_apply_moves(game, game->moves, n);
	if (err < 0) {
		/* ERROR_QUIT means we've won. */
		if (err == -ERROR_QUIT)
			printf("%s Wins!\n", game_current_colour(game));
		return err;
	}

	/* Move game to RUNNING. */
	game->state = RUNNING;

	puts("Game Ready");
	printf("%s's Turn\n", game_current_colour(game));
	return 0;
}
