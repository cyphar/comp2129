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
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "worm.h"

size_t g_nthreads = 3;

/* Used for debugging a given struct book_t. */
#if defined(DEBUG)
static void pr_book_t(struct book_t *book)
{
	fprintf(stderr, "book[%lu] aid=%lu pid=%lu ae=[", book->id, book->author_id, book->publisher_id);
	for (size_t i = 0; i < book->n_author_edges; i++)
		fprintf(stderr, "%lu%s", book->b_author_edges[i], i < book->n_author_edges - 1 ? " " : "");
	fprintf(stderr, "] ce=[");
	for (size_t i = 0; i < book->n_citation_edges; i++)
		fprintf(stderr, "%lu%s", book->b_citation_edges[i], i < book->n_citation_edges - 1 ? " " : "");
	fprintf(stderr, "] pe=[");
	for (size_t i = 0; i < book->n_citation_edges; i++)
		fprintf(stderr, "%lu%s", book->b_citation_edges[i], i < book->n_citation_edges - 1 ? " " : "");
	fprintf(stderr, "]\n");
}
#endif

struct search_arg_t {
	struct book_t *nodes;
	ssize_t start, end;
	size_t to_find;
	ssize_t *ret;
};

/*
 * Because binary is being compiled with -O0 switches are an extra cmp branch
 * when generating a function for the three variants we care about it arguably
 * more efficient. .text space is cheap.
 */
#define DEFUN_SEARCH(fn, field)							\
	void *fn(void *args)							\
	{									\
		struct search_arg_t *arg = args;				\
		for (ssize_t i = arg->start; i < arg->end; i++)	{		\
			if (arg->nodes[i].field == arg->to_find) {		\
				*arg->ret = i;					\
				break;						\
			}							\
		}								\
		return NULL;							\
	}

DEFUN_SEARCH(search_bid, id);
DEFUN_SEARCH(search_aid, author_id);
DEFUN_SEARCH(search_pid, publisher_id);

typedef void *(*thread_func_t)(void *);

enum {
	SEARCH_BOOK,
	SEARCH_AUTHOR,
	SEARCH_PUBLISHER,
};

/*
 * The only real optimisation to most of the operations is making the lookup of
 * a particular id occur in parallel.
 *
 * TODO: This is slower than search_linear even with count=1e6.
 */
struct book_t *search_parallel(struct book_t *nodes, size_t count, int type, size_t val)
{
	ssize_t idx = -1;
	size_t nthreads = g_nthreads - 1;

	pthread_t *threads = malloc(nthreads * sizeof(*threads));
	struct search_arg_t *args = malloc(nthreads * sizeof(*args));

	/* Pick search function. */
	thread_func_t fn;
	switch (type) {
	case SEARCH_BOOK:
		fn = search_bid;
		break;
	case SEARCH_AUTHOR:
		fn = search_aid;
		break;
	case SEARCH_PUBLISHER:
		fn = search_pid;
		break;
	default:
		return NULL;
	}

	/* Spin up threads. */
	for (size_t i = 0; i < nthreads; i++) {
		/* Separate as usual. */
		size_t start = ( i    * count) / nthreads;
		size_t end   = ((i+1) * count) / nthreads;
		if (end > count)
			end = count;

		args[i] = (struct search_arg_t) {
			.nodes = nodes,
			.to_find = val,
			.ret = &idx,
			.start = start,
			.end = end,
		};

		pthread_create(threads + i, NULL, fn, args + i);
	}

	/* Wait for threads. */
	for (size_t i = 0; i < nthreads; i++)
		pthread_join(threads[i], NULL);

	/* Return the node. */
	if (idx < 0)
		return NULL;
	return &nodes[idx];
}

/* Linear searches are sometimes more efficient due to thread overhead. */
struct book_t *search_linear(struct book_t *nodes, size_t count, int type, size_t val)
{
	/* Create a "dummy" search_arg_t, and don't use any threads. */
	ssize_t idx = -1;
	struct search_arg_t arg = {
		.nodes = nodes,
		.to_find = val,
		.start = 0,
		.end = count,
		.ret = &idx,
	};

	/* Do the search. */
	switch (type) {
	case SEARCH_BOOK:
		search_bid(&arg);
		break;
	case SEARCH_AUTHOR:
		search_aid(&arg);
		break;
	case SEARCH_PUBLISHER:
		search_pid(&arg);
		break;
	}

	/* Return the node. */
	if (idx < 0)
		return NULL;
	return &nodes[idx];
}

/* Front-end to searching that decides whether parallelism is worth it. */
struct book_t *do_search(struct book_t *nodes, size_t count, int type, size_t val)
{
	return search_linear(nodes, count, type, val);
}

/**
 * find_book - Finds a book with the given id in the set of nodes.
 * @nodes: node list from graph
 * @count: size of node list
 * @book_id: book being searched for
 */
struct result_t *find_book(struct book_t *nodes, size_t count, size_t book_id)
{
	struct result_t *result = malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));

	/* We only want a single book. */
	struct book_t *book = do_search(nodes, count, SEARCH_BOOK, book_id);
	if (book) {
		result->elements = malloc(++result->n_elements * sizeof(*result->elements));
		result->elements[0] = book;
	}
	return result;
}

/**
 * find_books_by_author - Finds books written by the given author
 * @nodes: node list from graph
 * @count: size of node list
 * @author_id: author being searched for
 */
struct result_t *find_books_by_author(struct book_t *nodes, size_t count,
				      size_t author_id)
{
	struct result_t *result = malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));

	struct book_t *source_book = do_search(nodes, count, SEARCH_AUTHOR, author_id);
	if (!source_book)
		return result;

	/* Pre-allocate to move allocation out of the hot loop. */
	result->n_elements = 1 + source_book->n_author_edges;
	result->elements = malloc(result->n_elements * sizeof(*result->elements));

	/*
	 * The result are all of the indices in ->b_author_edges and also the
	 * source_book itself. It's not clear how SIMD could help here due to the
	 * redirect in the index information.
	 */
	for (size_t i = 0; i < source_book->n_author_edges; i++)
		result->elements[i] = &nodes[source_book->b_author_edges[i]];
	result->elements[source_book->n_author_edges] = source_book;

	return result;
}

/**
 * find_books_reprinted - Finds books reprinted by a different publisher
 * @nodes: node list from graph
 * @count: size of node list
 * @publisher_id: initial publisher
 */
struct result_t *find_books_reprinted(struct book_t *nodes, size_t count,
				      size_t publisher_id)
{
	struct result_t *result = malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));
	result->elements = malloc(count * sizeof(*result->elements));

	struct book_t *source_book = do_search(nodes, count, SEARCH_PUBLISHER, publisher_id);
	if (!source_book)
		return result;

	/*
	 * Get the ->b_publisher_edges for the given publisher_id, giving us the
	 * full set of publisher indexes.
	 */
	size_t n_publisher_edges = source_book->n_publisher_edges;
	size_t *b_publisher_edges = malloc((n_publisher_edges + 1) * sizeof(*b_publisher_edges));
	memcpy(b_publisher_edges, source_book->b_publisher_edges, n_publisher_edges * sizeof(*b_publisher_edges));
	b_publisher_edges[n_publisher_edges++] = (source_book - nodes);

	/*
	 * Collect ->b_author_edges for each book in the books by the publisher.
	 * This aspect of the spec is _incredibly_ underspecified and has been
	 * broken several times in the past. But this is far more optimal than the
	 * "naive" way of iterating over the entire graph and checking against all
	 * publisher edges.
	 */
	for (size_t i = 0; i < n_publisher_edges; i++) {
		struct book_t *book = &nodes[b_publisher_edges[i]];

		/* ->b_author_edges will never contain itself. */
		for (size_t j = 0; j < book->n_author_edges; j++)
			if (nodes[book->b_author_edges[j]].id == book->id)
				/* result->elements is always big enough. */
				result->elements[result->n_elements++] = &nodes[book->b_author_edges[j]];
	}

	free(b_publisher_edges);
	return result;
}

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

/**
 * find_books_k_distance - Finds books k distance away
 * @nodes: node list from graph
 * @count: size of node list
 * @book_id: source book
 * @k: distance
 *
 * This is effectively a simplified Djikstra's algorithm, with pruning when
 * evaluating nodes that are "out of scope".
 */
struct result_t *find_books_k_distance(struct book_t *nodes, size_t count,
				       size_t book_id, uint16_t k)
{
	/* The result will never be larger than */
	struct result_t *result = malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));
	/* Pre-allocate the elements, as the returned result cannot be larger than count. */
	result->elements = malloc(count * sizeof(*result->elements));

	/*
	 * The elements in in struct result_t are always pointers inside nodes, thus we
	 * can just take the pointer difference to get the index.
	 */
	struct book_t *book = do_search(nodes, count, SEARCH_BOOK, book_id);
	if (!book)
		return result;
	size_t idx = book - nodes;

	/*
	 * The "best_k" list. This is all we care about, and is used for values in
	 * the @remaining queue. Note that because part of the priority queue's
	 * data is shared with us in this array, we must only modify entries that
	 * won't cause the p_queue_t to become corrupted.
	 */
	uint32_t *best_k = malloc(count * sizeof(*best_k));
	for (size_t i = 0; i < count; i++)
		best_k[i] = UINT32_MAX;
	best_k[idx] = 0;

	/* Create stack for our search. We really don't care about the order of iteration. */
	struct p_queue_t *remaining = pqueue_alloc(count);
	pqueue_insert(remaining, best_k, idx);

	while (!pqueue_empty(remaining)) {
		/*
		 * NOTE: We have to be very careful when touching best_k. We can only
		 * touch entries that aren't inside remaining (entries that have had
		 * ownership transferred to us).
		 */
		size_t idx = pqueue_remove(remaining, best_k);
		struct book_t *book = &nodes[idx];

		/*
		 * There's no way the children will be interesting if the best path to
		 * this node already hits k, prune.
		 */
		if (best_k[idx] >= k)
			continue;

		/* Iterate over all children and see whether we've found a better value. */
		for (size_t i = 0; i < book->n_citation_edges; i++) {
			size_t nxt = book->b_citation_edges[i];
			uint32_t tentative = best_k[idx] + 1;

			if (tentative < best_k[nxt]) {
				best_k[nxt] = tentative;
				pqueue_increase(remaining, best_k, nxt);
			}
		}
	}

	/* Every node with a best_k <= k is what we want. */
	for (size_t i = 0; i < count; i++)
		if (best_k[i] <= k)
			result->elements[result->n_elements++] = &nodes[i];

	free(best_k);
	pqueue_free(remaining);
	return result;
}

struct queue_t {
	size_t *vector;
	size_t size;
	size_t head, tail;
};

static struct queue_t *queue_alloc(size_t size)
{
	struct queue_t *queue = malloc(sizeof(*queue));
	if (!queue)
		return NULL;

	queue->size = size;
	queue->vector = malloc(queue->size * sizeof(*queue->vector));
	if (!queue->vector) {
		free(queue);
		return NULL;
	}

	queue->head = queue->tail = 0;
	return queue;
}

static void queue_free(struct queue_t *queue)
{
	if (queue->vector)
		free(queue->vector);
	free(queue);
}

static inline void queue_enqueue(struct queue_t *queue, size_t val)
{
	queue->vector[queue->tail++ % queue->size] = val;
}

static inline ssize_t queue_dequeue(struct queue_t *queue)
{
	return queue->vector[queue->head++ % queue->size];
}

static inline int queue_empty(struct queue_t *queue)
{
	return queue->head == queue->tail;
}


/**
 * find_shortest_distance - Finds the shortest path between two books
 * @nodes: node list from graph
 * @count: size of node list
 * @b1_id: start book
 * @b2_id: end book
 */
struct result_t *find_shortest_distance(struct book_t *nodes, size_t count, size_t b1_id, size_t b2_id)
{
	struct book_t *b1, *b2;
	size_t b1_idx, b2_idx;

	/* Used for BFS. */
	uint8_t *seen;
	struct queue_t *queue;
	ssize_t *previous;

	struct result_t *result = malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));

	b1 = do_search(nodes, count, SEARCH_BOOK, b1_id);
	if (!b1)
		return result;
	b1_idx = b1 - nodes;

	/*
	 * TODO: We have to ignore b1_idx in order to try to find a b2_idx that is
	 *       different. It'll have to be done with an ignore mask, but it'll be
	 *       ugly.
	 */

	b2 = do_search(nodes, count, SEARCH_BOOK, b2_id);
	if (!b2)
		return result;
	b2_idx = b2 - nodes;

	/* Set of nodes seen. We use char because it's guaranteed to be only one byte. */
	seen = malloc(count * sizeof(*seen));
	memset(seen, 0, count * sizeof(*seen));
	/* Vector of previous nodes, used to build the final path. */
	previous = malloc(count * sizeof(*previous));
	for (size_t i = 0; i < count; i++)
		previous[i] = -1;
	/* Queue used for BFS, storing the indices. */
	queue = queue_alloc(count);
	queue_enqueue(queue, b1_idx);

	/* BFS */
	size_t current;
	while (!queue_empty(queue)) {
		current = queue_dequeue(queue);
		if (current == b2_idx)
			break;
		for (size_t i = 0; i < nodes[current].n_author_edges; i++) {
			size_t idx = nodes[current].b_author_edges[i];

			if (seen[idx])
				continue;

			seen[idx] = 1;
			previous[idx] = current;
			queue_enqueue(queue, idx);
		}
		for (size_t i = 0; i < nodes[current].n_citation_edges; i++) {
			size_t idx = nodes[current].b_citation_edges[i];

			if (seen[idx])
				continue;

			seen[idx] = 1;
			previous[idx] = current;
			queue_enqueue(queue, idx);
		}
		for (size_t i = 0; i < nodes[current].n_publisher_edges; i++) {
			size_t idx = nodes[current].b_publisher_edges[i];

			if (seen[idx])
				continue;

			seen[idx] = 1;
			previous[idx] = current;
			queue_enqueue(queue, idx);
		}
	}

	/* Path not found, bail with empty results. */
	if (current != b2_idx)
		goto out;

	/* Create the path. */
	current = b2_idx;
	while (current != b1_idx) {
		result->elements = realloc(result->elements, ++result->n_elements * sizeof(*result->elements));
		result->elements[result->n_elements-1] = &nodes[current];
		current = previous[current];
	}
	result->elements = realloc(result->elements, ++result->n_elements * sizeof(*result->elements));
	result->elements[result->n_elements-1] = &nodes[b2_idx];

	/* Reverse in-place. */
	for (size_t i = 0; i < result->n_elements / 2; i++) {
		struct book_t *tmp = result->elements[result->n_elements - i - 1];
		result->elements[result->n_elements - i - 1] = result->elements[i];
		result->elements[i] = tmp;
	}

out:
	free(seen);
	free(previous);
	queue_free(queue);
	return result;
}

/* Needed to get the tests to run. */
struct result_t *find_shortest_edge_type(struct book_t *nodes, size_t count, size_t a1_id, size_t a2_id)
{
	return NULL;
}
