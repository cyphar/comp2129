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
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "locker.h"

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

int queue_enqueue(struct queue_t *queue, struct locker_t *locker)
{
	queue->head = realloc(queue->head, ++queue->size * sizeof(*queue->head));
	memmove(queue->head+1, queue->head, (queue->size-1) * sizeof(*queue->head));
	queue->head[0] = locker;
	debug("enqueue: %p[%u] -> %p\n", locker, locker->id, queue->head);
	return 0;
}

struct locker_t *queue_dequeue(struct queue_t *queue)
{
	/* No need to realloc here. */
	if (!queue->size)
		return NULL;
	debug("dequeue: %p <- %p[%lu]\n", queue->head[queue->size-1], queue->head, queue->size-1);
	return queue->head[--queue->size];
}

struct queue_t *queue_alloc(void)
{
	struct queue_t *queue = malloc(sizeof(struct queue_t));
	if (!queue)
		return NULL;
	memset(queue, '\0', sizeof(struct queue_t));
	return queue;
}

void queue_free(struct queue_t *queue)
{
	if (queue)
		free(queue->head);
	free(queue);
}

struct locker_t *locker_new(void)
{
	struct locker_t *locker = malloc(sizeof(*locker));
	if (!locker)
		return NULL;
	memset(locker, '\0', sizeof(struct locker_t));
	*locker = (struct locker_t){
		.fd = -1,
		.alive = false,
	};
	return locker;
}

void locker_free(struct locker_t *locker)
{
	if (locker->fd >= 0)
		close(locker->fd);
	free(locker);
}

int locker_get_state(struct locker_t *locker, struct locker_state_t *state)
{
	struct locker_msg_t msg = {
		.type = MSG_GET_STATE,
		.arg = {0},
	};

	if (write(locker->fd, &msg, sizeof(msg)) != sizeof(msg))
		return -1;
	if (read(locker->fd, &msg, sizeof(msg)) != sizeof(msg))
		return -1;

	memcpy(state, &msg.arg.state, sizeof(*state));
	return 0;
}

int locker_kill(struct locker_t *locker)
{
	struct locker_msg_t msg = {
		.type = MSG_DELETE,
		.arg  = {0},
	};

	if (write(locker->fd, &msg, sizeof(msg)) < 0)
		return -1;
	/*
	 * TODO: We should probably wait(2) here, but it's not obvious whether the
	 *       tester will be messing around with our children. Since they use
	 *       ptrace(2), waitpid might act in odd ways.
	 */
	locker->alive = false;
	return 0;
}

int manager_CREATE(struct manager_state_t *state, int argc, char **argv)
{
	struct locker_t *locker;
	int sockfd[2];

	if (argc != 1)
		return -1;

	/* Create a new locker slot. */
	state->lockers = realloc(state->lockers, ++state->lockers_length * sizeof(*state->lockers));
	locker = locker_new();
	locker->id = state->lockers_length;
	state->lockers[state->lockers_length-1] = locker;

	/* Add the locker to the queue. */
	queue_enqueue(state->queue, locker);

	/* Spawn the locker. */
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) < 0)
		return -1;

	locker->fd = sockfd[0];
	locker->alive = true;

	/* Before we fork, flush buffers so the child doesn't try to write anything. */
	fflush(stdout);
	fflush(stderr);
	locker->pid = fork();

	if (locker->pid < 0)
		goto err_enomem;

	/* In the child. */
	if (locker->pid == 0) {
		/*
		 * We return a positive to tell our parent to free memory and spin in
		 * the locker loop. To be honest, the cleanest way of doing this would
		 * be execve("/proc/self/exe").
		 */
		close(sockfd[0]);
		return sockfd[1];
	}

	/* Parent. */
	close(sockfd[1]);
	printf("New Locker Created: %u\n", locker->id);
	return 0;

err_enomem:
	fprintf(stderr, "Cannot Build Locker\n");
	return -1;
}

int manager_DELETE(struct manager_state_t *state, int argc, char **argv)
{
	uint16_t id;
	char *endptr;
	struct locker_t *locker;

	if (argc != 2)
		return -1;

	id = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr)
		goto err_enoent;
	if (id <= 0 || id > state->lockers_length)
		goto err_enoent;
	locker = state->lockers[id-1];
	if (!locker->alive)
		goto err_enoent;

	if (locker_kill(locker) < 0)
		goto err_enoent;
	printf("Locker %u Removed\n", locker->id);
	return 0;

err_enoent:
	fprintf(stderr, "Locker Does Not Exist\n");
	return -1;
}

void manager_print_state(uint16_t id, struct locker_state_t *state)
{
	printf("Locker ID: %u\n", id);
	printf("Lock Status: %slocked\n", state->locked ? "" : "un");
	if (!state->owned)
		printf("Owner: unowned\n");
	else
		printf("Owner: %u\n", state->owner_id);
}

int manager_QUERY(struct manager_state_t *state, int argc, char **argv)
{
	uint16_t id;
	char *endptr;
	struct locker_t *locker;
	struct locker_state_t locker_state;

	if (argc != 2)
		goto err_enoent;

	id = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr)
		goto err_enoent;
	if (id <= 0 || id > state->lockers_length)
		goto err_enoent;
	locker = state->lockers[id-1];
	if (!locker->alive)
		goto err_enoent;

	if (locker_get_state(locker, &locker_state) < 0)
		goto err_enoent;
	manager_print_state(id, &locker_state);
	return 0;

err_enoent:
	fprintf(stderr, "Locker Does Not Exist\n");
	return -1;
}

int manager_QUERYALL(struct manager_state_t *state, int argc, char **argv)
{
	if (argc != 1)
		return -1;

	for (uint16_t i = 0; i < state->lockers_length; i++) {
		struct locker_t *locker;
		struct locker_state_t locker_state;

		locker = state->lockers[i];
		if (!locker->alive)
			continue;
		if (locker_get_state(locker, &locker_state) < 0)
			continue;
		if (i)
			puts("");
		manager_print_state(i+1, &locker_state);
	}

	return 0;
}

int manager_LOCK(struct manager_state_t *state, int argc, char **argv)
{
	uint16_t id;
	char *endptr;
	struct locker_t *locker;

	if (argc != 2)
		goto err_enoent;

	id = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr)
		goto err_enoent;
	if (id <= 0 || id > state->lockers_length)
		goto err_enoent;

	/*
	 * We don't remove lockers that fail to send because we're told we can
	 * assume this won't happen.
	 */
	locker = state->lockers[id-1];
	if (!locker->alive)
		goto err_enoent;
	if (kill(locker->pid, SIGUSR1) < 0)
		goto err_enoent;
	printf("Locker %d Locked\n", id);
	return 0;

err_enoent:
	fprintf(stderr, "Locker Does Not Exist\n");
	return -1;
}

int manager_UNLOCK(struct manager_state_t *state, int argc, char **argv)
{
	uint16_t id;
	char *endptr;
	struct locker_t *locker;

	if (argc != 2)
		goto err_enoent;

	id = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr)
		goto err_enoent;
	if (id <= 0 || id > state->lockers_length)
		goto err_enoent;

	/*
	 * We don't remove lockers that fail to send because we're told we can
	 * assume this won't happen.
	 */
	locker = state->lockers[id-1];
	if (!locker->alive)
		goto err_enoent;
	if (kill(locker->pid, SIGUSR2) < 0)
		goto err_enoent;
	printf("Locker %d Unlocked\n", id);
	return 0;

err_enoent:
	fprintf(stderr, "Locker Does Not Exist\n");
	return -1;
}

int manager_ATTACH(struct manager_state_t *state, int argc, char **argv)
{
	uint16_t owner_id;
	char *endptr;
	struct locker_t *locker;
	struct locker_msg_t msg = {
		.type = MSG_SET_OWNER,
		.arg = {0},
	};

	if (argc != 2)
		return -1;

	owner_id = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr)
		goto err_einval;
	if (owner_id <= 0)
		goto err_einval;

	do {
		locker = queue_dequeue(state->queue);
		if (!locker)
			goto err_enomem;
	} while (!locker->alive);

	msg.arg.owner_id = owner_id;
	if (write(locker->fd, &msg, sizeof(msg)) != sizeof(msg))
		return -1;

	printf("Locker %u Owned By %u\n", locker->id, owner_id);
	return 0;

err_enomem:
	fprintf(stderr, "No Lockers Available\n");
	return -1;

err_einval:
	fprintf(stderr, "Invalid Argument\n");
	return -1;
}

int manager_DETACH(struct manager_state_t *state, int argc, char **argv)
{
	uint16_t id;
	char *endptr;
	struct locker_t *locker;
	struct locker_msg_t msg = {
		.type = MSG_REMOVE_OWNER,
		.arg = {0},
	};

	if (argc != 2)
		return -1;

	id = strtol(argv[1], &endptr, 10);
	if (endptr == argv[1] || *endptr)
		goto err_einval;
	if (id <= 0 || id > state->lockers_length)
		goto err_enoent;

	locker = state->lockers[id-1];
	if (!locker->alive)
		goto err_enoent;
	if (write(locker->fd, &msg, sizeof(msg)) != sizeof(msg))
		return -1;
	if (queue_enqueue(state->queue, locker) < 0)
		goto err_enomem;

	printf("Locker %u Unowned\n", locker->id);
	return 0;

err_enoent:
	fprintf(stderr, "Locker Does Not Exist\n");
	return -1;

err_enomem:
	fprintf(stderr, "Memory Error\n");
	return -1;

err_einval:
	fprintf(stderr, "Invalid Argument\n");
	return -1;
}

int manager_QUIT(struct manager_state_t *state, int argc, char **argv)
{
	if (argc != 1)
		return -1;

	for (int i = 0; i < state->lockers_length; i++) {
		struct locker_t *locker = state->lockers[i];

		if (!locker->alive)
			continue;
		locker_kill(locker);
	}

	state->quit = true;
	return 0;
}

int main(int argc, char **argv)
{
	struct manager_state_t state = {
		.queue = queue_alloc(),
		.lockers = NULL,
		.lockers_length = 0,
		.quit = false,
	};

	while (!state.quit) {
		struct cmd_t *cmd = NULL;
		char *line = readline();
		if (!line)
			continue;

		cmd = cmd_alloc();
		if (!cmd)
			continue;
		if (cmd_parse(cmd, line) < 0)
			goto next;

		/* CMD_CREATE is special since it forks. */
		if (cmd->ident == CMD_CREATE) {
			int res = manager_CREATE(&state, argc, argv);
			if (res <= 0)
				/* Parent. */
				goto next;

			/* We are in the child so we have to free everything. */
			struct locker_state_t child_state = {
				.owned = false,
				.locked = true,
				.owner_id = 0,
			};

			cmd_free(cmd);
			free(line);
			queue_free(state.queue);
			for (uint16_t i = 0; i < state.lockers_length; i++)
				locker_free(state.lockers[i]);
			free(state.lockers);

			exit(locker_main(&child_state, res));
		}

		switch (cmd->ident) {
#define CMD(ident)															\
			case CMD_##ident: {												\
					int res = manager_##ident(&state, cmd->argc, cmd->argv); \
					if (res < 0)											\
						{} /* XXX: Should we do something here? */			\
				}															\
				break;
#include "command_list.h"
#undef CMD
			case END_CMDS:
			default:
				goto next;
		}

next:
		if (!state.quit)
			puts("");
		cmd_free(cmd);
		free(line);
	}

	queue_free(state.queue);
	for (uint16_t i = 0; i < state.lockers_length; i++)
		locker_free(state.lockers[i]);
	free(state.lockers);
	return 0;
}
