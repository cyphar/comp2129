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
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

static sem_t even_sem;
static sem_t odd_sem;

void setup_semaphores(void) __attribute__((constructor));
void setup_semaphores(void)
{
	sem_init(&even_sem, 0, 0);
	sem_init(&odd_sem, 0, 1);
}

void *thread_even(void *arg)
{
	int end = (intptr_t) arg;

	for (int i = 2; i <= end; i += 2) {
		sem_wait(&even_sem);
		printf("%d\n", i);
		sem_post(&odd_sem);
	}

	return NULL;
}

void *thread_odd(void *arg)
{
	int end = (intptr_t) arg;

	for (int i = 1; i <= end; i += 2) {
		sem_wait(&odd_sem);
		printf("%d\n", i);
		sem_post(&even_sem);
	}

	return NULL;
}
