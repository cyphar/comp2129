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

#pragma once

#ifndef LOCKER_H
#define LOCKER_H

#include <stdint.h>
#include <sys/types.h>
#include <signal.h>

#if defined(DEBUG)
#	define debug(...) \
	do { fprintf(stderr, __VA_ARGS__); } while (0)
#else
#	define debug(...)
#endif

/* Represents the current state of a particular locker. */
struct locker_state_t {
	uint16_t owner_id;
	uint8_t locked;
	uint8_t owned;
};

/* Manager <-> Locker protocol. */
struct locker_msg_t {
	enum {
		MSG_DELETE,
		MSG_GET_STATE,
		MSG_SET_OWNER,
		MSG_REMOVE_OWNER,
		/* We set the lock state using SIGUSR[12]. */
		MSG_REPLY,
	} type;

	union {
		uint8_t unused;              /* MSG_{DELETE,GET_STATE,REMOVE_OWNER} */
		uint16_t owner_id;           /* MSG_SET_OWNER */
		struct locker_state_t state; /* MSG_REPLY */
	} arg;
};

/*
 * Represents the state of a locker process (from the perspective of a
 * manager). There is no information about the current lock state stored here
 * [because lockers can be modified out-of-band].
 */
struct locker_t {
	uint16_t id;

	/* Whether a given locker slot is still "alive". */
	uint8_t alive;

	/* The pid of a locker. */
	pid_t pid;

	/* Used for non-signal-based IPC. */
	int fd;
};

struct locker_t *locker_new(void);
void locker_free(struct locker_t *locker);
int locker_main(struct locker_state_t *state, int sockfd);

/* Current locker freelist. Linux kernel's list_head would be better... */
struct queue_t {
	struct locker_t **head;
	size_t size;
};

/* Commands. */
enum {
#define CMD(ident) CMD_##ident,
#include "command_list.h"
#undef CMD
	END_CMDS,
};

/* State of the manager. */
struct manager_state_t {
	struct queue_t *queue;
	struct locker_t **lockers;
	uint16_t lockers_length;
	uint8_t quit;
};

#define CMD(ident) \
	int manager_##ident(struct manager_state_t *state, int argc, char **argv);
#include "command_list.h"
#undef CMD

/* cmd_t stores the parsed state of a command string. */
struct cmd_t {
	int ident;
	int argc;
	char **argv;
};

/* malloc and free equivalents for cmd_t. */
struct cmd_t *cmd_alloc(void);
void cmd_free(struct cmd_t *cmd);

/*
 * Parses the provided line and fills the command data. cmd must have already
 * been allocated with cmd_alloc. Return value is < 0 if an error occurred.
 * cmd->argv[0] is the name of the command.
 */
int cmd_parse(struct cmd_t *cmd, char *line);

#endif /* !defined(LOCKER_H) */
