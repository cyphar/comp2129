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

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * Before I begin, this code is *so* much easier to implement in Python. Having
 * to reimplement itertools.permutation and itertools.product is just annoying.
 */

/* Basic stack implementation. We only need to implement a numerical stack. */
struct stack_t {
	size_t size;
	double *elements;
};

/* Allocate stack. */
struct stack_t *stack_alloc(void)
{
	struct stack_t *stack = malloc(sizeof(struct stack_t));
	if (!stack)
		return NULL;

	stack->size = 0;
	stack->elements = NULL;
	return stack;
}

/* Free stack. */
void stack_free(struct stack_t *stack)
{
	free(stack->elements);
	free(stack);
}

/* Push value to stack, expanding if necessary. */
int stack_push(struct stack_t *stack, double value)
{
	stack->elements = realloc(stack->elements, ++stack->size * sizeof(*stack->elements));
	if (!stack->elements)
		return -1;
	stack->elements[stack->size-1] = value;
	return 0;
}

/* Pop value from stack. */
int stack_pop(struct stack_t *stack, double *value)
{
	if (stack->size <= 0)
		return -1;

	*value = stack->elements[stack->size-1];
	stack->size--;
	return 0;
}

/* Returns the current length of the stack (not its allocated size). */
size_t stack_size(struct stack_t *stack)
{
	return stack->size;
}

/* Represents an RPN operator in an expression. */
typedef double (*operator_t)(struct stack_t *stack);

#define op_define(name, op, cond) \
	double op_##name(struct stack_t *stack) \
	{ \
		double left, right; \
		if (stack_pop(stack, &right) < 0) \
			return -1; \
		if (stack_pop(stack, &left) < 0) \
			return -1; \
		if (cond) \
			return -1; \
		return stack_push(stack, left op right); \
	}

/* Define the operators. */
op_define(add, +, false);
op_define(sub, -, false);
op_define(mul, *, false);
op_define(div, /, right == 0);

/* Make a list of the defined operators. */
static operator_t OPERATORS[] = {
	op_add,
	op_sub,
	op_mul,
	op_div,
};
#define NUM_OPERATORS (sizeof(OPERATORS) / sizeof(*OPERATORS))

/*
 * A reverse polish notation "expression element". This is only used when
 * generating RPN expressions.
 */
struct expr_elem_t {
	enum {
		TYPE_NUMBER,
		TYPE_OPERATOR,
	} type;

	union {
		int value;       /* TYPE_NUMBER */
		operator_t func; /* TYPE_OPERATOR */
	};
};

/*
 * Evaluate an RPN expression, setting the given result pointer to the
 * evaluated value. If an error occurs during the evaluation, -1 will be
 * returned.
 */
int expr_eval(struct expr_elem_t *expr, size_t n, double *result)
{
	int err = -1;
	struct stack_t *stack = stack_alloc();
	if (!stack)
		goto out;

	for (size_t i = 0; i < n; i++) {
		struct expr_elem_t *elem = &expr[i];
		switch (elem->type) {
			case TYPE_NUMBER:
				if (stack_push(stack, elem->value) < 0)
					goto out;
				break;
			case TYPE_OPERATOR:
				if (elem->func(stack) < 0)
					goto out;
				break;
			default:
				goto out;
		}
	}

	/* There should only be one element left on the stack. */
	if (stack_size(stack) != 1)
		goto out;
	if (stack_pop(stack, result) < 0)
		goto out;

	err = 0;
out:
	stack_free(stack);
	return err;
}

/* Simple macro to swap a and b. This requires -std=gnu99. */
#define swap(a, b) \
	do { \
		typeof(a) _tmp = (a); \
		(a) = (b); \
		(b) = _tmp; \
	} while (0)

#define EPSILON 1e-8
bool double_eq(double left, double right)
{
	return fabs(left - right) < EPSILON;
}

/*
 * Modified version of Heap's algorithm for generating permutations
 * recursively. It's a bit ugly because of the need to separately store the
 * size of the expression and the "unpermuted prefix index" as well. Users
 * should set @n = @size.
 */
bool expr_reachable_permute(struct expr_elem_t *expr, size_t size, size_t n, double target)
{
	/*
	 * Base case, there are no elements left to swap, just compute the
	 * expression. The need to check if the expression is valid could be
	 * optimised by generating the only valid "template" expressions beforehand
	 * but that code is only really nice to implement in Python (where you have
	 * iterators).
	 */
	if (n <= 1) {
		double value;
		if (expr_eval(expr, size, &value) < 0)
			return false;
		/* Compare integers... */
		return double_eq(value, target);
	}

	/* Swap elements to generate the permutations. */
	for (size_t i = 0; i < n - 1; i++) {
		if (expr_reachable_permute(expr, size, n - 1, target))
			return true;

		if (n % 2)
			swap(expr[0], expr[n-1]);
		else
			swap(expr[i], expr[n-1]);
	}
	return expr_reachable_permute(expr, size, n - 1, target);
}

/*
 * Checks whether the given target is reachable from a permutation of the given
 * RPN expression (invalid permutations are ignored).
 */
bool expr_reachable(struct expr_elem_t *expr, size_t size, double target)
{
	return expr_reachable_permute(expr, size, size, target);
}

/*
 * Iterates a given array of indices through all possible combinations
 * (effectively performing a cartesian power of the array [0..@n-1] with an
 * index of @s). Returns false if all combinations have been generated -- note
 * that this function MUST be called with an array of zeros initially.
 *
 * Users should follow code like this:
 *   // s is the number of indices to iterate over, n is the length of the
 *   // array the indices are iterating over.
 *   size_t *idxs = calloc(s, sizeof(size_t));
 *   do {
 *     // Your code.
 *   } while (cartesian_next(idxs, s, n));
 *   free(idxs);
 */
bool cartesian_next(size_t *idxs, size_t s, size_t n)
{
	/*
	 * Treat the array of indexes as the digits of a base-n number that is
	 * being incremented (little "endian" order), with a carry bit. If the
	 * iteration would overflow, we have exhausted all combinations.
	 */
	size_t i = 0;
	bool carry = false;

	do {
		carry = ++idxs[i] >= n;
		idxs[i] %= n;
	} while (++i < s && carry);

	/* If carry is still set at the end, we have exausted all cases. */
	return !carry;
}

bool ascending(size_t *numbers, size_t n)
{
	for (size_t i = 1; i < n; i++)
		if (numbers[i] < numbers[i-1])
			return false;
	return true;
}

/*
 * Reachable returns whether the given set of @numbers can reach (with the set
 * of %OPERATORS, repetitions allowed) the given target.
 */
bool reachable(int *numbers, size_t n, double target)
{
	/* For n elements, we need n-1 operators to reduce the stack to one element. */
	size_t expr_len = 2*n - 1;
	bool found = false;
	struct expr_elem_t *expr = NULL;
	size_t *idxs = NULL;

	expr = malloc(sizeof(struct expr_elem_t) * expr_len);
	if (!expr)
		goto out;

	/*
	 * Iterate over the cartesian power (n-1) of indexes in the OPERATORS
	 * array to generate the set of possible RPN expression tokens. This is
	 * actually overkill because expr_reachable does a permutation of the
	 * tokens, so we could reduce this iteration by using similar tricks to the
	 * "handshake problem" in discrete maths (only allow ascending idxs for
	 * example and skip the others).
	 */
	idxs = calloc(n - 1, sizeof(size_t));
	if (!idxs)
		goto out;

	do {
		/* Ignore non-ascending indices. */
		if (!ascending(idxs, n - 1))
			continue;

		/* Fill the first n entries with the numbers. */
		for (size_t i = 0; i < n; i++) {
			expr[i].type = TYPE_NUMBER;
			expr[i].value = (double) numbers[i];
		}

		/* Fill the last n-1 entries with the current set of operators. */
		for (size_t i = 0; i < expr_len - n; i++) {
			expr[n+i].type = TYPE_OPERATOR;
			expr[n+i].func = OPERATORS[idxs[i]];
		}

		/* Is the target reachable from this expression set? */
		found |= expr_reachable(expr, expr_len, target);
	} while (!found && cartesian_next(idxs, n - 1, NUM_OPERATORS));

out:
	free(expr);
	free(idxs);
	return found;
}

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

/* Defined by problem. */
#define INPUT_SIZE 4
#define TARGET 24

int parse_digits(char *line, int *values, size_t n)
{
	size_t i = 0;

	for (char *p = line; *p != '\0' && i < n; p++) {
		char *endptr;

		/* Skip over blanks. */
		while (isspace(*p))
			p++;
		if (*p == '\0')
			break;

		/* Add to values. */
		values[i] = strtol(p, &endptr, 10);
		if (!isspace(*endptr) || endptr == p)
			break;

		/* Update iterator. */
		p = endptr;
		i++;

		/* Null terminate. */
		while (!isspace(*p) && *p != '\0')
			p++;
		if (*p == '\0')
			break;
		*p = '\0';
	}

	if (i != n - 1)
		return -1;
	return 0;
}

int main(void)
{
	int values[INPUT_SIZE];
	char *fmt, *line = NULL;

	printf("Enter %d integers: ", INPUT_SIZE);
	fflush(stdout);

	line = readline();
	puts("");
	if (!line)
		goto bad_format;
	if (parse_digits(line, values, INPUT_SIZE) < 0)
		goto bad_format;
	for (size_t i = 0; i < INPUT_SIZE; i++)
		if (values[i] == 0)
			goto bad_format;
	free(line);

	if (reachable(values, INPUT_SIZE, TARGET))
		fmt = "Yes! %d is reachable from { %d, %d, %d, %d }\n";
	else
		fmt = "Noooo :( %d is unreachable from { %d, %d, %d, %d }\n";

	printf(fmt, TARGET, values[0], values[1], values[2], values[3]);
	return 0;

bad_format:
	puts("Input must consist of 4 integers");
	free(line);
	return 1;
}
