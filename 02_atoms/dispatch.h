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

#if !defined(COMMANDS_H)
#define COMMANDS_H

#include "atoms.h"

enum {
#define CMD(ident) CMD_##ident,
#include "command_list.h"
#undef CMD
	END_CMDS,
};

/* cmd_t stores the parsed state of a command string. */
struct cmd_t {
	int ident;
	int argc;
	char **argv;
};

/* malloc and free equivalents for cmd_t. */
struct cmd_t *cmd_alloc(void);
void cmd_free(struct cmd_t *cmd);

/*
 * Parses the provided line and fills the command data. cmd must have already
 * been allocated with cmd_alloc. Return value is < 0 if an error occurred.
 * cmd->argv[0] is the name of the command.
 */
int cmd_parse(struct cmd_t *cmd, char *line);

/*
 * Entrypoint to dispatching calls for various commands, which are called based
 * on the .cmd value of the command passed. Return value is < 0 if an error
 * occurred.
 */
int cmd_dispatch(struct cmd_t *cmd, struct game_t *game);

/* List of commands. */
#define CMD(ident) int __cmd_##ident(struct cmd_t *cmd, struct game_t *game);
#include "command_list.h"
#undef CMD

#endif /* !defined(COMMANDS_H) */
