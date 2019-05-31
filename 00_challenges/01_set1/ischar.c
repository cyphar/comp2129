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

int main(void)
{
	char *input;
	int value;

	input = readline();
	value = atoi(input);
	free(input);

	printf("Is %d\n", value);
	printf("Alpha-Numeric?\n");

	if (isalnum(value))
		printf("Yes, it is '%c'\n", value);
	else
		printf("No, it is '%c'\n", value);

	return 0;
}

