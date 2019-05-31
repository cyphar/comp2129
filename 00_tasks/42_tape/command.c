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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tape.h"

static char const *COMMANDS[] = {
#define CMD(ident) [CMD_##ident] = #ident,
#include "command_list.h"
#undef CMD
	NULL,
};

/* Creates a new cmd_t, returns NULL in case of allocation failure. */
struct cmd_t *cmd_alloc(void)
{
	struct cmd_t *cmd = malloc(sizeof(struct cmd_t));
	if (!cmd)
		return NULL;

	*cmd = (struct cmd_t) {
		.ident = END_CMDS,
		.argc = 0,
		.argv = NULL,
	};
	return cmd;
}

/* Frees a cmd_t allocated by cmd_alloc. Noop if cmd is NULL. */
void cmd_free(struct cmd_t *cmd)
{
	if (cmd)
		free(cmd->argv);
	free(cmd);
}

/*
 * Parses @argv0 and returns the indentifier of the command associated with the
 * string name. Return value is < 0 if an error occurred.
 */
static int cmd_ident(char *argv0)
{
	for (int i = 0; i < END_CMDS; i++) {
		if (!strcmp(COMMANDS[i], argv0))
			return i;
	}
	return -1;
}

/*
 * Parses the provided line and fills the command data. @cmd must have already
 * been allocated with cmd_alloc. Return value is < 0 if an error occurred.
 * @line will be modified by this function.
 */
int cmd_parse(struct cmd_t *cmd, char *line)
{
	if (!cmd || !line)
		return -1;

	/* Reset to defaults. */
	free(cmd->argv);
	*cmd = (struct cmd_t) {
		.ident = END_CMDS,
		.argc = 0,
		.argv = NULL,
	};

	for (char *p = line; *p != '\0'; p++) {
		/* Skip over blanks. */
		while (isspace(*p))
			p++;
		if (*p == '\0')
			break;

		/* Add to argv. */
		cmd->argv = realloc(cmd->argv, ++cmd->argc * sizeof(char *));
		cmd->argv[cmd->argc-1] = p;

		/* Null terminate. */
		while (!isspace(*p) && *p != '\0')
			p++;
		if (*p == '\0')
			break;
		*p = '\0';
	}

	/* Need at least one argc. */
	if (cmd->argc < 1)
		return -1;

	int ident = cmd_ident(cmd->argv[0]);
	if (ident < 0)
		return ident;

	cmd->ident = ident;
	return 0;
}
