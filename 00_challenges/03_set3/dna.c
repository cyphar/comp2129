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

char complement(char ch)
{
	char ret = 'x';

	switch (toupper(ch)) {
	case 'A':
		ret = 'T';
		break;
	case 'T':
		ret = 'A';
		break;
	case 'G':
		ret = 'C';
		break;
	case 'C':
		ret = 'G';
		break;
	}

	if (islower(ch))
		ret = tolower(ret);
	return ret;
}

int main(void)
{
	char *str = NULL;
	int err = 1;

	printf("Enter strand: ");
	fflush(stdout);

	str = readline();
	if (!str || !strlen(str))
		goto out;

	for (char *p = str; *p; p++)
		*p = complement(*p);

	err = 0;

out:
	if (!str)
		puts("");
	puts("");
	if (!err)
		printf("Complementary strand is %s\n", str);
	else
		puts("No strand provided.");

	free(str);
	return 0;
}
