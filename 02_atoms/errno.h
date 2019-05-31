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

#pragma once

#if !defined(ERRNO_H)
#define ERRNO_H

#define ERROR_QUIT               1 /* Signifies that the game should exit. */
#define ERROR_INTERNAL           2 /* An error that is not defined by the spec. */
#define ERROR_INVALID_COMMAND    3 /* Invalid Command */
#define ERROR_INVALID_ARGS       4 /* Invalid command arguments */
#define ERROR_CANNOT_START       5 /* Cannot Start Game */
#define ERROR_MISSING_ARGUMENTS  6 /* Mising Argument */
#define ERROR_EXCESS_ARGUMENTS   7 /* Too Many Arguments */
#define ERROR_GAME_NOT_RUNNING   8 /* Game Not In Progress */
#define ERROR_RESTART_GAME       9 /* Restart Application To Load Save */
#define ERROR_CANNOT_LOAD       10 /* Cannot Load Save */
#define ERROR_FILE_EXISTS       11 /* File Already Exists */
#define ERROR_INVALID_TURN      12 /* Invalid Turn Number */
#define ERROR_CANNOT_PLACE      13 /* Cannot Place Atom Here */
#define ERROR_INVALID_COORDS    14 /* Invalid Coordinates */
#define ERROR_CANNOT_UNDO       15 /* Cannot Undo */

/* Like perror but for atom commands. */
void atom_perror(int err);

#endif /* !defined(ERRNO_H) */
