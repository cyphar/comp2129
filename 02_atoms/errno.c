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

#include "errno.h"

/* Like perror but for atom commands. */
void atom_perror(int err)
{
	char *str = "[INTERNAL] Unknown Error";

	if (err >= 0)
		return;

	switch (-err) {
		case ERROR_QUIT:
			/* Nothing to print. */
			return;
		case ERROR_INTERNAL:
			str = "Internal Error";
			break;
		case ERROR_INVALID_COMMAND:
			str = "Invalid Command";
			break;
		case ERROR_INVALID_ARGS:
			str = "Invalid command arguments";
			break;
		case ERROR_CANNOT_START:
			str = "Cannot Start Game";
			break;
		case ERROR_MISSING_ARGUMENTS:
			str = "Missing Argument";
			break;
		case ERROR_EXCESS_ARGUMENTS:
			str = "Too Many Arguments";
			break;
		case ERROR_GAME_NOT_RUNNING:
			str = "Game Not In Progress";
			break;
		case ERROR_RESTART_GAME:
			str = "Restart Application To Load Save";
			break;
		case ERROR_CANNOT_LOAD:
			str = "Cannot Load Save";
			break;
		case ERROR_FILE_EXISTS:
			str = "File Already Exists";
			break;
		case ERROR_INVALID_TURN:
			str = "Invalid Turn Number";
			break;
		case ERROR_CANNOT_PLACE:
			str = "Cannot Place Atom Here";
			break;
		case ERROR_INVALID_COORDS:
			str = "Invalid Coordinates";
			break;
		case ERROR_CANNOT_UNDO:
			str = "Cannot Undo";
			break;
	}

	puts(str);
}
