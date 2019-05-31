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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

#define GRID_WIDTH  3
#define GRID_HEIGHT 3

char check_winner(char *grid)
{
	/*
	 * 0|1|2
     * -----
     * 3|4|5
     * -----
     * 6|7|8
	 */

	if (grid[0] && grid[0] == grid[1] && grid[1] == grid[2])
		return grid[0];
	else if (grid[3] && grid[3] == grid[4] && grid[4] == grid[5])
		return grid[3];
	else if (grid[6] && grid[6] == grid[7] && grid[7] == grid[8])
		return grid[6];
	else if (grid[0] && grid[0] == grid[3] && grid[3] == grid[6])
		return grid[0];
	else if (grid[1] && grid[1] == grid[4] && grid[4] == grid[7])
		return grid[1];
	else if (grid[2] && grid[2] == grid[5] && grid[5] == grid[8])
		return grid[2];
	else if (grid[0] && grid[0] == grid[4] && grid[4] == grid[8])
		return grid[0];
	else if (grid[2] && grid[2] == grid[4] && grid[4] == grid[6])
		return grid[2];
	return '\0';
}

bool check_draw(char *grid)
{
	for (size_t i = 0; i < GRID_WIDTH*GRID_HEIGHT; i++)
		if (!grid[i])
			return false;
	return true;
}

void display_grid(char *grid)
{
	puts("");

	for (size_t y = 0; y < GRID_HEIGHT; y++) {
		for (size_t x = 0; x < GRID_WIDTH; x++) {
			char ch = grid[GRID_WIDTH*y+x] ? grid[GRID_WIDTH*y+x] : ' ';
			char end = (x == GRID_WIDTH-1) ? '\n' : '|';
			printf("%c%c", ch, end);
		}
		if (y != GRID_HEIGHT-1) {
			for (size_t x = 0; x < GRID_WIDTH*2-1; x++)
				putchar('-');
			puts("");
		}
	}
	puts("");
}

#define NUM_PLAYERS 2

int main(void)
{
	/* Game state. */
	char grid[GRID_WIDTH*GRID_HEIGHT] = {0};
	char players[NUM_PLAYERS] = {'X', 'O'};
	size_t current_player = 0;

	while (true) {
		int x, y;
		char *line = readline();
		if (!line)
			goto retry;
		if (sscanf(line, "%d %d", &x, &y) != 2)
			goto retry;
		if (x < 0 || x > 2)
			goto retry;
		if (y < 0 || y > 2)
			goto retry;

		grid[GRID_WIDTH*y+x] = players[current_player];
		current_player = (current_player + 1) % NUM_PLAYERS;

		if (!check_winner(grid) && !check_draw(grid))
			display_grid(grid);
retry:
		free(line);
		if (check_winner(grid) || check_draw(grid))
			break;
	}

	if (check_winner(grid))
		printf("%c wins!\n", check_winner(grid));
	else
		printf("Draw\n");
	display_grid(grid);
	return 0;
}
