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

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "tape.h"

#define bail(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

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

/* Implements python-like %. */
int mod(int a, int b)
{
	while (a < 0)
		a += b;
	return a % b;
}

struct reader_state_t {
	int idx;
	char *data;
	size_t length;
	int base;
	int to_read;
};

void *tape_reader(void *args)
{
	char *filename;
	int fd;
	struct reader_state_t *state = args;

	int idx = 0;
	int diff = 2 * (state->to_read > 0) - 1;

	if (asprintf(&filename, "./head%d", state->idx + 1) < 0)
		return NULL;

	fd = open(filename, O_WRONLY|O_APPEND|O_CREAT, 0644);
	if (fd < 0)
		return NULL;
	free(filename);

	idx = mod(state->base, state->length);
	while (state->to_read) {
		if (diff < 0)
			idx = mod(idx + diff, state->length);

		if (write(fd, &state->data[idx], sizeof(char)) < 0)
			goto out;

		if (diff > 0)
			idx = mod(idx + diff, state->length);

		state->to_read -= diff;
	}

out:
	fsync(fd);
	close(fd);
	return (void *)(intptr_t) idx;
}

struct tape_state_t {
	int *heads;
	size_t n_heads;
	char *data;
	ssize_t length;
};

void cmd_head(struct tape_state_t *state, struct cmd_t *cmd)
{
	int head;
	char *endptr;
	char *filename;

	if (cmd->argc != 2)
		return;
	head = strtol(cmd->argv[1], &endptr, 10);
	if (endptr == cmd->argv[1] || *endptr)
		return;

	state->heads = realloc(state->heads, ++state->n_heads * sizeof(*state->heads));
	state->heads[state->n_heads-1] = mod(head, state->length);

	/* Create output file. */
	if (asprintf(&filename, "./head%lu", state->n_heads) < 0)
		return;
	truncate(filename, 0);
	free(filename);

	printf("HEAD %lu at %+d\n\n", state->n_heads, head);
}

void cmd_read(struct tape_state_t *state, struct cmd_t *cmd)
{
	int read;
	char *endptr;
	struct reader_state_t *readers;
	pthread_t *threads;

	if (cmd->argc != 2)
		return;
	read = strtol(cmd->argv[1], &endptr, 10);
	if (endptr == cmd->argv[1] || *endptr)
		return;

	readers = malloc(state->n_heads * sizeof(*readers));
	threads = malloc(state->n_heads * sizeof(*threads));

	for (size_t i = 0; i < state->n_heads; i++) {
		readers[i].idx = i;
		readers[i].data = state->data;
		readers[i].length = state->length;
		readers[i].base = state->heads[i];
		readers[i].to_read = read;

		pthread_create(&threads[i], NULL, tape_reader, &readers[i]);
	}

	for (size_t i = 0; i < state->n_heads; i++) {
		void *retval;
		pthread_join(threads[i], &retval);
		state->heads[i] = (intptr_t) retval;
	}

	printf("Finished Reading\n\n");

	free(readers);
	free(threads);
}

int main(int argc, char **argv)
{
	int fd;

	struct cmd_t *cmd = NULL;
	bool quit = false;

	struct tape_state_t state = {0};

	if (argc < 2)
		bail("Tape Not Inserted\n");

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		bail("Cannot Read Tape\n");

	state.length = lseek(fd, 0, SEEK_END);
	if (state.length < 0)
		bail("Failed to seek\n");

	state.data = mmap(NULL, state.length, PROT_READ, MAP_SHARED, fd, 0);
	if (!state.data)
		bail("Failed to mmap\n");

	cmd = cmd_alloc();
	while (!quit) {
		char *line = readline();
		if (!line)
			continue;
		if (cmd_parse(cmd, line) < 0)
			goto next;

		switch (cmd->ident) {
			case CMD_HEAD:
				cmd_head(&state, cmd);
				break;
			case CMD_READ:
				cmd_read(&state, cmd);
				break;
			case CMD_QUIT:
				quit = true;
				break;
			default:
				goto next;
		}

next:
		free(line);
	}

	cmd_free(cmd);
	free(state.heads);
	munmap(state.data, state.length);
	close(fd);
	return 0;
}
