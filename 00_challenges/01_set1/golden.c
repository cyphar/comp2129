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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define bail(code, ...) \
	do { fprintf(stdout, __VA_ARGS__); exit(code); } while (0)

int main(void)
{
	double a, b;

	printf("Enter two numbers: ");
	int n = scanf("%lf %lf", &a, &b);
	puts("");
	if (n != 2)
		bail(1, "Invalid input.\n");

	/* A must always be greater than B. */
	if (a < b) {
		double tmp = a;
		a = b;
		b = tmp;
	}

	double lhs = (a + b) / a;
	double rhs = a / b;

	if (fabs(lhs - rhs) <= 0.001)
		puts("Golden ratio!");
	else
		puts("Maybe next time.");

	return 0;
}
