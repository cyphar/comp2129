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
#include <stdbool.h>
#include <string.h>

#include "atoms.h"

/*
 * Gets a new line from stdin, caller responsible for calling free on returned
 * string. NULL is returned iff EOF was hit without any characters being read.
 */
char *readline(void)
{
	size_t len = 0;
	char *line = NULL;

	while (true) {
		char ch = getchar();
		if (ch == EOF || ch == '\n')
			ch = '\0';

		line = realloc(line, ++len * sizeof(char));
		line[len-1] = ch;

		if (!ch)
			break;
	}

	/*
	 * If we have a string of length 0 and we hit an EOF, we've hit an EOF
	 * without any trailing characters on the last line.
	 */
	if (strlen(line) == 0 && feof(stdin)) {
		free(line);
		return NULL;
	}

	return line;
}

int main(void)
{
	struct cmd_t *cmd = NULL;
	struct game_t *game = NULL;

	cmd = cmd_alloc();
	if (!cmd)
		goto err_free;

	game = game_alloc();
	if (!game)
		goto err_free;

	while (true) {
		int err = -ERROR_INTERNAL;
		char *line = readline();
		if (!line)
			break;

		err = cmd_parse(cmd, line);
		if (err < 0)
			goto next;

		err = cmd_dispatch(cmd, game);
		if (err < 0)
			goto next;

		/* TODO: Do we need to have a state update step here. */

		/* Clear error value, everything went well. */
		err = 0;
next:
		free(line);
		if (err == -ERROR_QUIT)
			break;
		if (err < 0)
			atom_perror(err);
		/*
		 * Not mentioned in the spec, but apparently we have to output a
		 * newline after each command for some reason...
		 */
		printf("\n");
		fflush(stdout);
	}

	game_free(game);
	cmd_free(cmd);
	return 0;

err_free:
	game_free(game);
	cmd_free(cmd);
	bail(1, "error allocating memory\n");
	return 1;
}
