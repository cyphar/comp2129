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

#define die(...) \
	do { printf(__VA_ARGS__); exit(1); } while(0)

/* Compute Manhattan distance. */
int manhattan(int x1, int y1, int x2, int y2)
{
	return abs(x2 - x1) + abs(y2 - y1);
}

void grids_are_hard(int width, int height, int xpos, int ypos, int limit)
{
	/* Header. */
	for (int i = 0; i < width*2+1; i++) {
		if (i == 0 || i == width*2)
			printf("+");
		else
			printf("-");
	}
	printf("\n");

	/* Actual lines. */
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int distance = manhattan(xpos, ypos, x, y);

			if (xpos == x && ypos == y)
				printf("|C");
			else if (distance <= limit)
				printf("|%d", limit - distance);
			else
				printf("| ");
		}
		printf("|\n");
	}

	/* Footer. */
	for (int i = 0; i < width*2+1; i++) {
		if (i == 0 || i == width*2)
			printf("+");
		else
			printf("-");
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	char *endptr = NULL;
	int width, height, x, y, limit;

	if (argc != 6)
		die("Invalid arguments.\n");

	width = strtol(argv[1], &endptr, 10);
	if (!endptr || *endptr != '\0' || width < 0)
		die("Invalid Width\n");

	height = strtol(argv[2], &endptr, 10);
	if (!endptr || *endptr != '\0' || height < 0)
		die("Invalid Height\n");

	x = strtol(argv[3], &endptr, 10);
	if (!endptr || *endptr != '\0' || x < 0 || x >= width)
		die("Invalid Character Properties\n");

	y = strtol(argv[4], &endptr, 10);
	if (!endptr || *endptr != '\0' || y < 0 || y >= height)
		die("Invalid Character Properties\n");

	limit = strtol(argv[5], &endptr, 10);
	if (!endptr || *endptr != '\0' || limit < 0)
		die("Invalid Character Properties\n");

	grids_are_hard(width, height, x, y, limit);
	return 0;
}
