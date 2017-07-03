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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nonogram.h"

char *readline(void)
{
	size_t len = 0;
	char *line = NULL;

	for (;;) {
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

void free_nonogram(struct nonogram_t n)
{
	for (int i = 0; i < n.x_runs_length; i++)
		free(n.x_runs[i].run);
	free(n.x_runs);
	for (int i = 0; i < n.y_runs_length; i++)
		free(n.y_runs[i].run);
	free(n.y_runs);
}

int main(void)
{
	struct nonogram_t nonogram = {0};
	char **grid = NULL;
	int err = 1, xlen, ylen;

	char *line = readline();
	if (!line || sscanf(line, "%d %d", &ylen, &xlen) != 2) {
		printf("Cannot decode\n");
		goto out;
	}
	if (xlen <= 0 || ylen <= 0) {
		printf("Cannot decode\n");
		goto out;
	}
	nonogram.x_runs_length = xlen;
	nonogram.x_runs = calloc(nonogram.x_runs_length, sizeof(struct run_t));
	nonogram.y_runs_length = ylen;
	nonogram.y_runs = calloc(nonogram.y_runs_length, sizeof(struct run_t));

	/* Read in the grid. */
	int i = 0;
	grid = calloc(nonogram.x_runs_length, sizeof(char *));
	for (;;) {
		char *line = readline();
		if (!line)
			break;

		/* Too many lines. */
		if (i >= nonogram.x_runs_length) {
			free(line);
			break;
		}

		grid[i] = line;

		/* Check length of string. */
		if ((int) strlen(grid[i]) != nonogram.y_runs_length) {
			printf("Invalid image data\n");
			goto out;
		}

		i++;
	}
	if (i != nonogram.x_runs_length) {
		printf("Invalid image data\n");
		goto out;
	}

	/* Compute the horizontals. */
	for (int y = 0; y < nonogram.x_runs_length; y++) {
		int x = nonogram.y_runs_length - 1, pos = 0;

		nonogram.x_runs[y].run = malloc(sizeof(int));
		nonogram.x_runs[y].run[pos] = 0;

		while (x >= 0) {
			if (grid[y][x] == '1')
				nonogram.x_runs[y].run[pos]++;
			else if (nonogram.x_runs[y].run[pos]) {
				pos++;
				nonogram.x_runs[y].run = realloc(nonogram.x_runs[y].run, (pos+1) * sizeof(int));
				nonogram.x_runs[y].run[pos] = 0;
			}
			x--;
		}

		if (!nonogram.x_runs[y].run[pos] && pos > 0)
			nonogram.x_runs[y].run = realloc(nonogram.x_runs[y].run, pos-- * sizeof(int));

		nonogram.x_runs[y].length = pos + 1;
	}

	/* Compute the verticals. */
	for (int x = 0; x < nonogram.y_runs_length; x++) {
		int y = nonogram.x_runs_length - 1, pos = 0;

		nonogram.y_runs[x].run = malloc(sizeof(int));
		nonogram.y_runs[x].run[pos] = 0;

		while (y >= 0) {
			if (grid[y][x] == '1')
				nonogram.y_runs[x].run[pos]++;
			else if (nonogram.y_runs[x].run[pos]) {
				pos++;
				nonogram.y_runs[x].run = realloc(nonogram.y_runs[x].run, (pos+1) * sizeof(int));
				nonogram.y_runs[x].run[pos] = 0;
			}
			y--;
		}

		if (!nonogram.y_runs[x].run[pos] && pos > 0)
			nonogram.y_runs[x].run = realloc(nonogram.y_runs[x].run, pos-- * sizeof(int));

		nonogram.y_runs[x].length = pos + 1;
	}

	/* Output the Xs. */
	puts("X:");
	for (int y = 0; y < nonogram.x_runs_length; y++) {
		for (int i = 0; i < nonogram.x_runs[y].length; i++) {
			printf("%d", nonogram.x_runs[y].run[i]);
			if (i != nonogram.x_runs[y].length - 1)
				printf(" ");
			else
				printf("\n");
		}
	}

	/* SPAAAAAAAACE. */
	puts("");

	/* Output the Ys. */
	puts("Y:");
	for (int x = 0; x < nonogram.y_runs_length; x++) {
		for (int i = 0; i < nonogram.y_runs[x].length; i++) {
			printf("%d", nonogram.y_runs[x].run[i]);
			if (i != nonogram.y_runs[x].length - 1)
				printf(" ");
			else
				printf("\n");
		}
	}

	err = 0;

out:
	/* Free everything. */
	free_nonogram(nonogram);
	for (int i = 0; i < nonogram.x_runs_length; i++)
		free(grid[i]);
	free(grid);
	free(line);

	return err;
}
