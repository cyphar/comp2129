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

#if !defined(TAPE_H)
#define TAPE_H

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

struct cmd_t *cmd_alloc(void);
void cmd_free(struct cmd_t *cmd);
int cmd_parse(struct cmd_t *cmd, char *line);

#endif /* !defined(TAPE_H) */
