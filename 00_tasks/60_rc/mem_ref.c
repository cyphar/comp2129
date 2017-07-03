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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#include "mem_ref.h"

/*
 * Straight-forward linked-list implementation. While on paper this is not as
 * efficient as a hashmap (especially for membership checking), this is fast
 * enough for our purposes. I wanted to implement this with RB trees, but it
 * seemed like massive overkill and hash-maps suffer from degredation and
 * resizing pains.
 */

struct ll_node_t {
	uintptr_t value;
	struct ll_node_t *next;
};

/* Allocate a new detached node. */
struct ll_node_t *ll_new(uintptr_t value)
{
	struct ll_node_t *node = malloc(sizeof(*node));
	if (!node)
		return NULL;

	node->value = value;
	node->next = NULL;
	return node;
}

/* Free a node, without detaching it from its relatives. */
void ll_free(struct ll_node_t *self)
{
	free(self);
}

/*
 * Append a node at the end of the list head given by @self. If @self is NULL,
 * a new node is created and @self will be updated to point to that new node.
 * If @value is already in the list, this operation will be a noop.
 */
void ll_append(struct ll_node_t **self, uintptr_t value)
{
	struct ll_node_t **slot = self;

	while (*slot && (*slot)->value != value)
		slot = &(*slot)->next;

	if (!*slot)
		*slot = ll_new(value);
}

/* Find and delete a node in the list with the given value. */
void ll_delete(struct ll_node_t **self, uintptr_t value)
{
	struct ll_node_t *old = NULL;
	struct ll_node_t **current = self;

	while (*current && (*current)->value != value)
		current = &(*current)->next;

	/* Not found. */
	if (!*current)
		return;

	old = *current;
	*current = (*current)->next;

	ll_free(old);
}

/* Search through the linked list for the given value. */
struct ll_node_t *ll_find(struct ll_node_t *self, uintptr_t value)
{
	while (self && self->value != value)
		self = self->next;
	return self;
}

/*
 * rc_t is a reference counted box, with hierarchical reference counting
 * explicitly encoded. Acquiring a reference to an rc_t will acquire references
 * to all ancestors, and likewise releasing a reference will release references
 * to all ancestors.
 */
struct rc_t {
	ssize_t count;
	struct rc_t *parent;

	/*
	 * This allows us to store the "box" at the end of the rc_t structure and
	 * be able to reference it directly. Pointers returned from *_ref are
	 * actually pointers to ->box fields of rc_t objects.
	 */
	char box[];
};

/*
 * This macro is heavily inspired by the container_of macro in the Linux
 * kernel. While it seems incredibly unsafe, this library will only call this
 * after verifying that pointers passed to *_ref are legitimate rc_t pointers.
 */
#define container_of(ptr, type, member) \
	((type *)((void *)(ptr) - offsetof(type, member)))

/*
 * The global BST is only used to track whether a caller to this library is
 * passing us garbage values, with entries only being the value of the box
 * pointer (&rc_t->box). Reference handling is stored with rc_t. Red-black, B-
 * or AVL trees would be more efficient, but require more case handling than I
 * care to write. Currently this is not MT-safe.
 */
static struct ll_node_t *root = NULL;

/*
 * Converts a given @ptr to a struct rc_t, verifying that the given ptr is a
 * valid rc_t box to avoid segmentation faults.
 */
struct rc_t *to_rc(void *ptr)
{
	if (!ll_find(root, (uintptr_t) ptr))
		return NULL;
	return container_of(ptr, struct rc_t, box);
}

void ll_purge(struct ll_node_t *self)
{
	while (self) {
		struct ll_node_t *tmp = self->next;

		/*
		 * In addition to freeing the entire list, we also free any remaining
		 * rc_t's as they obviously cannot have any at program exit. No need to
		 * bother with the hierarchies, since the entire tree is going to die.
		 */
		free(container_of(self->value, struct rc_t, box));
		ll_free(self);

		self = tmp;
	}
}

void root_cleanup(void) __attribute__((destructor));
void root_cleanup(void)
{
	ll_purge(root);
}

void rc_charge(struct rc_t *self)
{
	/* Assume all parents are still alive. */
	for (struct rc_t *p = self; p; p = p->parent)
		p->count++;
}

void rc_uncharge(struct rc_t *self)
{
	struct rc_t *current = self;

	while (current) {
		struct rc_t *parent = current->parent;

		if (--current->count <= 0) {
			ll_delete(&root, (uintptr_t) &current->box);
			free(current);
		}

		current = parent;
	}
}

void *new_ref(size_t size, void *dep)
{
	struct rc_t *new, *parent;

	parent = to_rc(dep);
	if (dep && !parent)
		return NULL;

	new = malloc(sizeof(*new) + size);
	if (!new)
		return NULL;

	new->count = 0;
	new->parent = parent;
	memset(&new->box, 0, size);

	/* Charge hierarchy. */
	rc_charge(new);

	/*
	 * Add &new->box to the BST. Note that we don't add @new, because that
	 * would require us to operate on an invalid memory address in order to
	 * check whether a pointer is a valid rc_t.
	 */
	ll_append(&root, (uintptr_t) &new->box);

	/* We return a pointer to the box. */
	return &new->box;
}

void *assign_ref(void *ref)
{
	struct rc_t *rc = to_rc(ref);

	/* Invalid rc. */
	if (!rc)
		return NULL;

	/* Charge hierarchy. */
	rc_charge(rc);
	return &rc->box;
}

void *del_ref(void *ref)
{
	struct rc_t *rc = to_rc(ref);

	/* Invalid rc. */
	if (!rc)
		return NULL;

	/* Uncharge hierarchy. */
	rc_uncharge(rc);

	/* If it is no longer in @root, then it has been freed. */
	if (!ll_find(root, (uintptr_t) ref))
		return NULL;
	return &rc->box;
}
