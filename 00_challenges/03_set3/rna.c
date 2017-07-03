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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
	char *str = NULL, *out = NULL, *p;
	int err = 1;

	printf("Input strand: ");
	fflush(stdout);

	str = readline();
	if (!str || !strlen(str))
		goto out;

	out = malloc((strlen(str) + 1) * sizeof(char));
	memset(out, '\0', (strlen(str) + 1) * sizeof(char));

	p = str;
	while (*p) {
		char *start, *end;

		start = strstr(p, "GUGU");
		if (!start)
			start = p + strlen(p);

		strncat(out, p, start - p);

		end = strstr(start, "AGAG");
		if (!end)
			break;
		p = end + strlen("AGAG");
	}

	err = 0;

out:
	puts("");
	if (!err)
		printf("Output is %s\n", out);
	else
		abort();

	free(str);
	free(out);
	return 0;
}
