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

char caesar(char ch, int key)
{
	if (!isalpha(ch))
		return ch;

	char cipher = (toupper(ch) - 'A' + key) % ('Z' - 'A' + 1) + 'A';
	if (islower(ch))
		cipher = tolower(cipher);
	return cipher;
}

int main(void)
{
	char *str = NULL, *endptr;
	int key = 0;
	int err = 1;

	printf("Enter key: ");
	fflush(stdout);

	str = readline();
	key = strtol(str, &endptr, 10);
	if (endptr == str || *endptr)
		goto out;
	if (key < 0 || key > 26)
		goto out;
	free(str);

	printf("Enter line: ");
	fflush(stdout);

	str = readline();
	if (!str || !strlen(str))
		goto out;

	for (char *p = str; *p; p++)
		*p = caesar(*p, key);

	err = 0;

out:
	puts("");

	if (!err)
		puts(str);
	else
		puts("Invalid key!");

	free(str);
	return 0;
}
