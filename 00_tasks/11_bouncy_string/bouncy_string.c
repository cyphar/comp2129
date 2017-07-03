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

#define die(...) \
	do { printf(__VA_ARGS__); exit(1); } while(0)

int main(int argc, char **argv)
{
	char *str, *endptr = NULL;
	int start, count, delta, i;

	if (argc != 4)
		die("Invalid arguments length\n");

	/* ./bouncy_string <str> <start> <count> */
	str = argv[1];
	if (strlen(str) < 1)
		die("Invalid string\n");

	start = strtol(argv[2], &endptr, 10);
	if (!endptr || *endptr != '\0' || start < 0 || start >= (int) strlen(str))
		die("Invalid start position\n");

	count = strtol(argv[3], &endptr, 10);
	if (!endptr || *endptr != '\0' || count < 0)
		die("Invalid iteration count\n");

	i = start;
	delta = 1;
	while (count-- >= 0) {
		fputc(str[i], stdout);

		if (i + delta < 0 || i + delta >= (int) strlen(str))
			delta = -delta;
		i += delta;
	}
	fputc('\n', stdout);
	return 0;
}
