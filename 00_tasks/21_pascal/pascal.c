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
#include <string.h>

#define bail(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

int main(int argc, char **argv)
{
	if (argc < 2)
		bail("Missing Argument\n");

	char *endptr;
	int rows = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr || rows < 0)
		bail("Invalid Argument\n");

	long *pascal_row = NULL;
	for (int row = 0; row <= rows; row++) {
		/* Create a new pascal_row copy, with 1 appended. */
		long *next = malloc((row + 1) * sizeof(*pascal_row));
		if (!next)
			bail("could not allocate\n");
		for (int i = 0; i < row; i++)
			next[i] = pascal_row[i];
		next[row] = 1;

		/* Update to compute pascal row. */
		for (int i = 1; i < row; i++)
			next[i] = pascal_row[i-1] + pascal_row[i];

		/* Print. */
		for (int i = 0; i <= row; i++)
			printf("%ld%c", next[i], i == row ? '\n' : ' ');

		/* Swap pascal_row for its copy. */
		free(pascal_row);
		pascal_row = next;
	}

	free(pascal_row);
	return 0;
}
