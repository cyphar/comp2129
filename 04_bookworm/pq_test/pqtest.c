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

#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* MIN-Priority Queue using array backing. */
struct p_queue_t {
	size_t *vector;
	ssize_t *inverse;
	size_t end;
};

static struct p_queue_t *pqueue_alloc(size_t size)
{
	struct p_queue_t *queue = malloc(sizeof(*queue));
	if (!queue)
		goto err;

	/* ->vector stores the actual queue. */
	queue->vector = malloc(size * sizeof(*queue->vector));
	if (!queue->vector)
		goto err_vec;

	/*
	 * ->inverse stores a mapping from book_idx -> vector_idx. This is done to
	 *  avoid linear lookups in pqueue_increase. Since the size of ->vector is
	 *  always the same as the size of the graph (this is guaranteed by the
	 *  caller) this is perfectly safe.
	 */
	queue->inverse = malloc(size * sizeof(*queue->vector));
	if (!queue->inverse)
		goto err_inv;

	queue->end = 0;
	memset(queue->vector, 0, size * sizeof(*queue->vector));
	for (size_t i = 0; i < size; i++)
		queue->inverse[i] = -1;
	return queue;

err_inv:
	free(queue->vector);
err_vec:
	free(queue);
err:
	return NULL;
}

static void pqueue_free(struct p_queue_t *queue)
{
	if (queue) {
		free(queue->vector);
		free(queue->inverse);
	}
	free(queue);
}

/*
 * Macros for indexing within the array. It's a binary heap, but stored as a
 * flat array to make cache access easier on the CPU.
 */
#define PQ_ROOT        0
#define PQ_OFFSET      1
#define PQ_LEFT(idx)   ((2*((idx) + PQ_OFFSET)) - PQ_OFFSET)
#define PQ_RIGHT(idx)  ((2*((idx) + PQ_OFFSET) + 1) - PQ_OFFSET)
#define PQ_PARENT(idx) ((((idx) + PQ_OFFSET) / 2) - PQ_OFFSET)

/*
 * Insert the the given index (with the corresponding value in @values) into
 * the priority queue.
 */
static inline void pqueue_insert(struct p_queue_t *queue, const uint32_t *values, size_t idx)
{
	/*
	 * We make a lot of assumptions to allow us to store the raw idx as well as
	 * not doing any extra allocations. In particular, we assume the caller
	 * isn't messing with us and won't change values that are owned by us. This
	 * isn't Rust so you can't express that restriction in code without locks.
	 */

	/* Insert at the end. */
	size_t slot = queue->end++;

	/* Propogate up the tree. */
	while (slot) {
		size_t parent = PQ_PARENT(slot);

		if (values[idx] > values[queue->vector[parent]])
			break;

		queue->vector[slot] = queue->vector[parent];
		queue->inverse[queue->vector[slot]] = slot;
		slot = parent;
	}

	queue->vector[slot] = idx;
	queue->inverse[idx] = slot;
}

/*
 * Increases the priority (decreases the value) of the given idx in the
 * priority queue. Note that the *only* value owned by queue that is allowed to
 * be changed by the caller is value[idx] -- and the value *must* have been
 * *decreased*.
 */
static void pqueue_increase(struct p_queue_t *queue, const uint32_t *values, size_t idx)
{
	/*
	 * Get the inverse vector and fall-back to a normal insert if the element
	 * is not present in the pqueue.
	 */
	ssize_t vecidx = queue->inverse[idx];
	if (vecidx < 0)
		return pqueue_insert(queue, values, idx);

	/*
	 * Bubble *up*. We know the direction because the caller told us they
	 * decreased the value.
	 */
	size_t slot = vecidx;
	while (slot) {
		size_t parent = PQ_PARENT(slot);

		if (values[idx] > values[queue->vector[parent]])
			break;

		queue->vector[slot] = queue->vector[parent];
		queue->inverse[queue->vector[slot]] = slot;
		slot = parent;
	}

	queue->vector[slot] = idx;
	queue->inverse[idx] = slot;
}

/*
 * Pop the highest value off the priority queue and reorder the queue as
 * necessary usign the provided values vector.
 */
static inline size_t pqueue_remove(struct p_queue_t *queue, const uint32_t *values)
{
	/* Get our return value first. */
	size_t slot = PQ_ROOT;
	size_t idx = queue->vector[slot];

	/* Deregister inverse. */
	queue->inverse[idx] = -1;

	/* We need to swap the end with the head. */
	queue->vector[slot] = queue->vector[--queue->end];
	size_t tmp = queue->vector[slot];

	/* Propogate the newest head down if necessary. */
	while (PQ_LEFT(slot) <= queue->end) {
		size_t left = PQ_LEFT(slot);
		size_t right = PQ_RIGHT(slot);

		size_t child = left;
		if (right < queue->end) {
			if (values[queue->vector[right]] < values[queue->vector[left]])
				child = right;
		}

		if (values[tmp] <= values[queue->vector[child]])
			break;

		queue->vector[slot] = queue->vector[child];
		queue->inverse[queue->vector[slot]] = slot;
		slot = child;
	}

	queue->vector[slot] = tmp;
	queue->inverse[tmp] = slot;
	return idx;
}

static inline int pqueue_empty(struct p_queue_t *queue)
{
	return !queue->end;
}

#define SIZE 10
int main(void)
{
	srand(time(NULL) | getpid());

	struct p_queue_t *queue = pqueue_alloc(SIZE);

	/* Generate random values. */
	uint32_t values[SIZE] = {0};
	for (size_t i = 0; i < SIZE; i++)
		values[i] = rand() % 100;

	/* Generate, making sure the invariants are followed. */
	for (size_t i = 0; i < SIZE; i++) {
		pqueue_insert(queue, values, i);
		/*
		for (size_t j = 0; j < queue->end; j++)
			printf("  add%lu  [%lu] -> %lu -> %lu\n", i, j, queue->vector[j], values[queue->vector[j]]);
		*/
	}
	for (size_t i = 0; i < SIZE; i++) {
		size_t val = pqueue_remove(queue, values);
		/*
		for (size_t j = 0; j < queue->end; j++)
			printf("  rem%lu  [%lu] -> %lu -> %lu\n", i, j, queue->vector[j], values[queue->vector[j]]);
		*/
		printf("[%lu] %lu -> %lu\n", queue->end, val, values[val]);
	}

	return 0;
}
