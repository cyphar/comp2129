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
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define bail(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

void *thread_even(void *arg);
void *thread_odd(void *arg);

int main(int argc, char **argv)
{
	int end;
	char *endptr;
	pthread_t even, odd;

	if (argc != 2)
		bail("No Argument Specified\n");

	end = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr)
		bail("No Argument Specified\n");
	if (end <= 0)
		bail("Argument is <= 0\n");

	pthread_create(&even, NULL, thread_even, (void *)(intptr_t) end);
	pthread_create(&odd, NULL, thread_odd, (void *)(intptr_t) end);

	pthread_join(even, NULL);
	pthread_join(odd, NULL);

	return 0;
}
