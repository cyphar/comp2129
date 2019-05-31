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
#include <stdlib.h>
#include <string.h>

#include "mem_ref.h"

int main(void)
{
	char *ptr1 = new_ref(32, NULL);
	char *ptr2 = new_ref(32, ptr1);
	char *ptr3 = new_ref(32, ptr1);
	char *ptr4 = new_ref(32, ptr2);
	char *ptr5 = new_ref(32, ptr4);

	ptr5 = assign_ref(ptr5);
	ptr4 = assign_ref(ptr5);
	ptr3 = assign_ref(ptr3);
	ptr3 = assign_ref(ptr3);
	ptr2 = assign_ref(ptr2);
	ptr1 = assign_ref(ptr1);

	printf("%p\n", ptr1);
	printf("%p\n", ptr2);
	printf("%p\n", ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr3, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);
	assign_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr3, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr3, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr3);

	strcpy(ptr1, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	strcpy(ptr2, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	strcpy(ptr4, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	strcpy(ptr5, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");

	del_ref(ptr5);
	del_ref(ptr4);

	return 0;
}
