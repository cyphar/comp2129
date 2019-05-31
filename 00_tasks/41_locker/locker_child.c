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
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>

#include "locker.h"

#define MAX_EVENTS 32

enum {
	FD_SIGFD,
	FD_SOCKFD,
};

int locker_handle_sockfd(struct locker_state_t *state, int sockfd)
{
	struct locker_msg_t msg = {0};

	if (read(sockfd, &msg, sizeof(msg)) != sizeof(msg))
		return -1;

	if (msg.type == MSG_DELETE)
		return -2;

	switch (msg.type) {
		case MSG_DELETE:
			return -2;
		case MSG_GET_STATE:
			msg.type = MSG_REPLY;
			msg.arg.state = *state;
			if (write(sockfd, &msg, sizeof(msg)) != sizeof(msg))
				return -1;
			break;
		case MSG_REMOVE_OWNER:
			state->owned = 0;
			state->owner_id = 0;
			break;
		case MSG_SET_OWNER:
			if (!state->owned) {
				state->owner_id = msg.arg.owner_id;
				state->owned = 1;
			}
			break;
		default:
			return -1;
	}

	return 0;
}

int locker_handle_sigfd(struct locker_state_t *state, int sigfd)
{
	struct signalfd_siginfo si = {0};

	if (read(sigfd, &si, sizeof(si)) != sizeof(si))
		return -1;

	switch (si.ssi_signo) {
		case SIGUSR1:
			debug("got sigusr1 from %d\n", si.ssi_pid);
			state->locked = 1;
			break;
		case SIGUSR2:
			debug("got sigusr2 from %d\n", si.ssi_pid);
			state->locked = 0;
			break;
		default:
			return -1;
	}

	return 0;
}

int locker_main(struct locker_state_t *state, int sockfd)
{
	int sigfd = -1;
	int epfd = -1;
	struct epoll_event event = {
		.events = EPOLLIN,
	};

	/* We listen for SIGUSR{1,2}. */
	sigset_t sigmask;
	if (sigemptyset(&sigmask) < 0)
		return -1;
	if (sigaddset(&sigmask, SIGUSR1) < 0)
		return -1;
	if (sigaddset(&sigmask, SIGUSR2) < 0)
		return -1;

	/* We need to block signal handlers to use signalfd(2). */
	if (sigprocmask(SIG_BLOCK, &sigmask, NULL) < 0)
		return -1;

	sigfd = signalfd(-1, &sigmask, 0);
	if (sigfd < 0)
		goto err;

	epfd = epoll_create1(0);
	if (epfd < 0)
		goto err;

	event.data.fd = sigfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sigfd, &event) < 0)
		goto err;
	event.data.fd = sockfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event) < 0)
		goto err;

	while (true) {
		struct epoll_event events[MAX_EVENTS] = {{0}};
		int nfd;

		nfd = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if (nfd <= 0)
			continue;

		for (int i = 0; i < nfd; i++) {
			struct epoll_event event = events[i];
			int ret;

			if (event.data.fd == sigfd)
				ret = locker_handle_sigfd(state, sigfd);
			else if (event.data.fd == sockfd)
				ret = locker_handle_sockfd(state, sockfd);
			else
				continue;

			if (ret == -2)
				goto out;
		}
	}

out:
	close(sigfd);
	close(sockfd);
	close(epfd);
	return 0;

err:
	close(sigfd);
	close(sockfd);
	close(epfd);
	return -1;
}
