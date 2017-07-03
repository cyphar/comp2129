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

#define die(...) \
	do { printf(__VA_ARGS__); exit(1); } while(0)

char *readline(void)
{
	int len = 0;
	char ch, *buf = NULL;

	while ((ch = getchar()) != '\n') {
		buf = realloc(buf, len+2);
		buf[len++] = ch;
	}

	buf[len] = '\0';
	return buf;
}

void zelda_is_the_princess(int nrows, int height)
{
	/****************\
	 *      /\      *
	 *     /  \     *
	 *    /____\    *
	 *   /\    /\   *
	 *  /  \  /  \  *
	 * /____\/____\ *
	\****************/

	int single_width = 2*height;
	int width = nrows * single_width;

	for (int row = 0; row < nrows; row++) {
		for (int line = 0; line < height; line++) {
			int start = height * (nrows - row) - (line + 1);
			int end = width - start;

			for (int x = 0; x < end; x++) {
				if (x < start)
					/* leftpad */
					printf(" ");
				else if (x % single_width == start % single_width)
					printf("/");
				else if (x % single_width == (end-1) % single_width)
					printf("\\");
				else
					/* floor */
					printf("%c", (line + 1) % height ? ' ' : '_');
			}
			printf("\n");
		}
	}
}

#define N_ROWS 2

int main(void)
{
	char *input, *endptr = NULL;
	int height;

	printf("Enter height: ");
	input = readline();
	height = strtol(input, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		free(input);
		die("Invalid height.\n");
	}
	free(input);
	if (height < 2 || height > 20)
		die("Invalid height.\n");

	puts("");
	zelda_is_the_princess(N_ROWS, height);
	return 0;
}
