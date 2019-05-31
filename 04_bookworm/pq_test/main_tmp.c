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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "worm.h"

/*
 * Gets a new line from stdin, caller responsible for calling free on returned
 * string. NULL is returned iff EOF was hit without any characters being read.
 */
char *readline(int fd)
{
	size_t len = 0;
	char *line = NULL;

	while (true) {
		char ch;
		int n = read(fd, &ch, sizeof(ch));
		if (n < 0)
			break;
		if (n == 0 || ch == '\n')
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

int record_load(char *line, size_t **b_edges, size_t *n_edges)
{
	char *current;
	size_t cap = 20;

	/* Start with nothing. */
	*b_edges = malloc(cap * sizeof(**b_edges));
	*n_edges = 0;

	/* We assume all records are well-formed. */
	current = strtok(line, " ");
	if (!current)
		goto out;
	do {
		char *endptr;
		size_t val = strtol(current, &endptr, 10);
		if (endptr == current || *endptr)
			goto err;

		if (*n_edges >= cap)
			*b_edges = realloc(*b_edges, (cap *= 2) * sizeof(**b_edges));
		(*b_edges)[(*n_edges)++] = val;
	} while ((current = strtok(NULL, " ")) != NULL);

out:
	return 0;

err:
	free(*b_edges);
	return -1;
}

void trimright(char *str)
{
	char *end = str + strlen(str) - 1;
	while (end >= str && isspace(*end))
		*end-- = '\0';
}

#define MAX_BUFFER 65536

/* Loads a given book graph. The format is the following (rows separated by
 * newlines, records separated with whitespace).
 *
 *   +------------+
 *   | book_count |
 *   +------------+
 *   | book_specs |
 *   |    ...     |
 *   +------------+
 *
 * With 'book_specs' being defined as.
 *
 *   +-------------------------+
 *   | ->id                    |
 *   +-------------------------+
 *   | ->publisher_id          |
 *   +-------------------------+
 *   | ->author_id             |
 *   +-------------------------+
 *   | ->b_publisher_edges ... |
 *   +-------------------------+
 *   | ->b_author_edges    ... |
 *   +-------------------------+
 *   | ->b_citation_edges  ... |
 *   +-------------------------+
 */
struct book_t *graph_load(char *filename, size_t *count)
{
	size_t n_books = 0;
	char line[MAX_BUFFER], *endptr;
	struct book_t *graph = NULL;

	FILE *f = fopen(filename, "r");
	if (!f) {
		perror("graph_load: open graph file");
		return NULL;
	}

	/* Read book_count. */
	if (!fgets(line, MAX_BUFFER, f))
		goto err_parsing;
	trimright(line);
	n_books = strtol(line, &endptr, 10);
	if (endptr == line || *endptr)
		goto err_parsing;

	graph = malloc(n_books * sizeof(*graph));

	/* Read all of the books. */
	for (size_t i = 0; i < n_books; i++) {
		/* Parse book->id. */
		if (!fgets(line, MAX_BUFFER, f))
			goto err_parsing;
		trimright(line);
		graph[i].id = strtol(line, &endptr, 10);
		if (endptr == line || *endptr)
			goto err_parsing;

		/* Parse book->publisher_id. */
		if (!fgets(line, MAX_BUFFER, f))
			goto err_parsing;
		trimright(line);
		graph[i].publisher_id = strtol(line, &endptr, 10);
		if (endptr == line || *endptr)
			goto err_parsing;

		/* Parse book->author_id. */
		if (!fgets(line, MAX_BUFFER, f))
			goto err_parsing;
		trimright(line);
		graph[i].author_id = strtol(line, &endptr, 10);
		if (endptr == line || *endptr)
			goto err_parsing;

		/* Parse book->b_publisher_edges. */
		if (!fgets(line, MAX_BUFFER, f))
			goto err_parsing;
		trimright(line);
		if (record_load(line, &graph[i].b_publisher_edges, &graph[i].n_publisher_edges) < 0)
			goto err_parsing;

		/* Parse book->b_author_edges. */
		if (!fgets(line, MAX_BUFFER, f))
			goto err_parsing;
		trimright(line);
		if (record_load(line, &graph[i].b_author_edges, &graph[i].n_author_edges) < 0)
			goto err_parsing;

		/* Parse book->b_citation_edges. */
		if (!fgets(line, MAX_BUFFER, f))
			goto err_parsing;
		trimright(line);
		if (record_load(line, &graph[i].b_citation_edges, &graph[i].n_citation_edges) < 0)
			goto err_parsing;
	}

	*count = n_books;
	return graph;

err_parsing:
	fprintf(stderr, "graph_load: failed to parse graph file\n");
	free(graph);
	return NULL;
}

#define HOW_OFTEN 87572 /* To make it much more sane to run on large graphs. */

/* Runs some benchmarks. */
void bench(struct book_t *graph, size_t count)
{
	printf("--find_book--\n");
	for (size_t i = 0; i < count; i += HOW_OFTEN / 50) {
		struct result_t *result = find_book(graph, count, graph[i].id);
		if (!result || result->n_elements != 1 || result->elements[0]->id != graph[i].id)
			fprintf(stderr, "Fail! wrong result for find_book!\n");
		/*
		else
			fprintf(stderr, "%lu -> %lu succeeded\n", i, i);
		*/
		printf(".");
		fflush(stdout);
		free(result->elements);
		free(result);
	}

	printf("\n--find_books_k_distance--\n");
	for (size_t i = 0; i < count; i += HOW_OFTEN / 10) {
		uint16_t num_k = 100;
		size_t *ks = malloc(num_k * sizeof(*ks));
		memset(ks, 0, num_k * sizeof(*ks));

		for (uint16_t k = 0; k < num_k; k++) {
			struct result_t *result = find_books_k_distance(graph, count, graph[i].id, k);
			ks[k] = result->n_elements;
			fprintf(stderr, "k=%u n_elements=%lu\n", k, result->n_elements);
			/*
			if (i % 1000 == 0) {
				printf(".");
				fflush(stdout);
			}
			*/
			free(result->elements);
			free(result);
		}

		/* Make sure it's the same between runs. */
		for (uint16_t k = 0; k < num_k; k++) {
			struct result_t *result = find_books_k_distance(graph, count, graph[i].id, k);
			if (ks[k] != result->n_elements)
				fprintf(stderr, "Fail! what? i=%lu k=%u old_k=%lu k=%lu\n", i, k, ks[k], result->n_elements);
			if (i % 1000 == 0) {
				printf(".");
				fflush(stdout);
			}
			free(result->elements);
			free(result);
		}

		/* Check it's ascending. */
		for (uint16_t k = 1; k < num_k; k++) {
			if (ks[k-1] > ks[k]) {
				if (i % 1000 == 0) {
					printf("?");
					fflush(stdout);
				}

				fprintf(stderr, "Fail! %lu %u -> n[%lu] < n-1[%lu]\n", i, k, ks[k], ks[k-1]);

				struct result_t *result_1 = find_books_k_distance(graph, count, graph[i].id, k);
				struct result_t *result_2 = find_books_k_distance(graph, count, graph[i].id, k-1);

				free(result_1->elements);
				free(result_1);
				free(result_2->elements);
				free(result_2);
			}
		}

		free(ks);
	}

	printf("\n--find_shortest_distance--\n");
	for (size_t i = 0; i < count; i += HOW_OFTEN) {
		for (size_t j = 0; j < count; j++) {
			if ((i + j) % HOW_OFTEN != 0)
				continue;

			struct result_t *result = find_shortest_distance(graph, count, graph[i].id, graph[j].id);
			/*
			fprintf(stderr, "shortest distance from %lu to %lu n_elements=%lu elements=[", graph[i].id, graph[j].id, result->n_elements);
			for (size_t k = 0; k < result->n_elements; k++)
				fprintf(stderr, "%lu+%lu%s", result->elements[k]->id, result->elements[k]->publisher_id, k < result->n_elements - 1 ? " " : "");
			fprintf(stderr, "]\n");
			if (j == i) {
				if (result->n_elements != 2)
					fprintf(stderr, "Fail! find_shortest_distance(%lu, %lu) failed to give n_elements == 1 [got %lu]\n", i, j, result->n_elements);
				else if (result->elements[0]->id != graph[i].id || result->elements[1]->id != graph[i].id)
					fprintf(stderr, "Fail! find_shortest_distance(%lu, %lu) != %lu [got %lu]\n", i, j, i, result->elements[0] - graph);
			}
			*/
			if (i % 1000 == 0) {
				printf(".");
				fflush(stdout);
			}
			free(result->elements);
			free(result);
		}
	}

	printf("\n--find_books_by_author--\n");
	for (size_t i = 0; i < count; i++) {
		struct result_t *result = find_books_by_author(graph, count, graph[i].author_id);
		/*
		fprintf(stderr, "find by author for aid=%lu n_elements=%lu elements=[", graph[i].author_id, result->n_elements);
		for (size_t k = 0; k < result->n_elements; k++)
			fprintf(stderr, "%lu+%lu%s", result->elements[k]->id, result->elements[k]->publisher_id, k < result->n_elements - 1 ? " " : "");
		fprintf(stderr, "]\n");
		*/
		if (i % 1000 == 0) {
			printf(".");
			fflush(stdout);
		}
		free(result->elements);
		free(result);
	}

	printf("\n--find_books_reprinted--\n");
	for (size_t i = 0; i < count; i += HOW_OFTEN / 10) {
		struct result_t *result = find_books_reprinted(graph, count, graph[i].publisher_id);
		/*
		fprintf(stderr, "books reprinted with pid=%lu n_elements=%lu elements=[", graph[i].publisher_id, result->n_elements);
		for (size_t j = 0; j < result->n_elements; j++)
			fprintf(stderr, "%lu+%lu%s", result->elements[j]->id, result->elements[j]->publisher_id, j < result->n_elements - 1 ? " " : "");
		fprintf(stderr, "]\n");
		*/
		if (i % 1000 == 0) {
			printf(".");
			fflush(stdout);
		}
		free(result->elements);
		free(result);
	}
}

int main(int argc, char **argv) {
	if (argc != 2)
		return -1;

	size_t count = 0;
	book_t* graph = graph_load(argv[1], &count);
	if (graph == NULL) {
		return 1;
	}

	bench(graph, count);

	for (size_t i = 0; i < count; i++) {
		free(graph[i].b_author_edges);
		free(graph[i].b_publisher_edges);
        free(graph[i].b_citation_edges);
	}
	free(graph);
	return 0;
}
