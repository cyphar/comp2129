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

#define bail(code, ...) \
	do { fprintf(stderr, __VA_ARGS__); exit(code); } while (0)

char tr(char ch, char *from, char *to, size_t maplen)
{
	for (size_t i = 0; i < maplen; i++) {
		if (from[i] == ch)
			return to[i];
	}
	return ch;
}

int main(int argc, char **argv)
{
	if (argc == 1)
		bail(1, "No arguments\n");
	if (argc > 3)
		bail(1, "Too many arguments\n");
	if (argc < 3)
		bail(1, "Not enough arguments\n");

	char *from = argv[1];
	char *to = argv[2];
	if (strlen(from) != strlen(to))
		bail(1, "Invalid arguments\n");
	size_t maplen = strlen(from);

	for (;;) {
		char ch = getchar();
		if (ch == EOF)
			break;

		char new = tr(ch, from, to, maplen);
		putchar(new);
	}

	return 0;
}
