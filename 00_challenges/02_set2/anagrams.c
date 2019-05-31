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
#include <ctype.h>

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

char anagram_filter(char ch)
{
	if (!isalnum(ch))
		return '\0';
	return tolower(ch);
}

int min(int a, int b)
{
	if (a > b)
		a = b;
	return a;
}

int compare_char(const void *leftp, const void *rightp)
{
	char left = * (const char *) leftp;
	char right = * (const char *) rightp;

	/*
	 * Flip the sign of the comparison if either of the characters are '\0' so
	 * that '\0' will go to the end rather than the start.
	 */
	bool flip = !left || !right;
	return (1 - 2*flip) * (left - right);
}

int main(void)
{
	char *line = NULL, *anagram = NULL;
	bool is_anagram = false;

	printf("Enter line: ");
	line = readline();
	if (!line)
		goto out;

	printf("Enter anagram: ");
	anagram = readline();
	if (!anagram)
		goto out;

	puts("");

	/* Save the length before we start adding '\0' everywhere. */
	size_t linelen = strlen(line);
	size_t anagramlen = strlen(anagram);

	/* Make strings lower case and ignore [^A-Za-z0-9]. */
	for (char *p = line; *p; p++)
		*p = anagram_filter(*p);
	for (char *p = anagram; *p; p++)
		*p = anagram_filter(*p);

	/* Sort both strings (ignored characters will go to the end). */
	qsort(line, linelen, sizeof(char), compare_char);
	qsort(anagram, anagramlen, sizeof(char), compare_char);

	/* Compare the two strings. They should be the same length now. */
	is_anagram = strcmp(line, anagram) == 0;

out:
	if (is_anagram)
		puts("Anagram!");
	else
		puts("Not an anagram.");

	free(line);
	free(anagram);
	return 0;
}
