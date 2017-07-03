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

#if !defined(WORM_H)
#define WORM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct book_t {
	size_t id;
	size_t author_id;
	size_t publisher_id;
	size_t *b_author_edges;
	size_t n_author_edges;
	size_t *b_citation_edges;
	size_t n_citation_edges;
	size_t *b_publisher_edges;
	size_t n_publisher_edges;
};

struct result_t {
	struct book_t **elements;
	size_t n_elements;
};

/* typedefs are evil. */
typedef struct book_t book_t;
typedef struct result_t result_t;

/* All of the interfaces required for the assignment. */
struct result_t *find_book(struct book_t *nodes, size_t count, size_t book_id);
struct result_t *find_books_by_author(struct book_t *nodes, size_t count, size_t author_id);
struct result_t *find_books_reprinted(struct book_t *nodes, size_t count, size_t publisher_id);
struct result_t *find_books_k_distance(struct book_t *nodes, size_t count, size_t book_id, uint16_t k);
struct result_t *find_shortest_distance(struct book_t *nodes, size_t count, size_t b1_id, size_t b2_id);

#endif

