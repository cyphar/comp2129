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
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#define CONTROLLER_DEV "controller0"

struct entry_t {
	uint32_t bytes;
};

#define ENTRY_LEFT(entry)  (((entry).bytes >>  0) & 0x01)
#define ENTRY_RIGHT(entry) (((entry).bytes >>  1) & 0x01)
#define ENTRY_UP(entry)    (((entry).bytes >>  2) & 0x01)
#define ENTRY_DOWN(entry)  (((entry).bytes >>  3) & 0x01)
#define ENTRY_X(entry)     (((entry).bytes >>  4) & 0x01)
#define ENTRY_Y(entry)     (((entry).bytes >>  5) & 0x01)
#define ENTRY_A(entry)     (((entry).bytes >>  6) & 0x01)
#define ENTRY_B(entry)     (((entry).bytes >>  7) & 0x01)
#define ENTRY_BK(entry)    (((entry).bytes >>  8) & 0x01)
#define ENTRY_ST(entry)    (((entry).bytes >>  9) & 0x01)
#define ENTRY_SE(entry)    (((entry).bytes >> 10) & 0x01)
#define ENTRY_MO(entry)    (((entry).bytes >> 11) & 0x01)
#define ENTRY_Z(entry)     (((entry).bytes >> 12) & 0x01)
#define ENTRY_W(entry)     (((entry).bytes >> 13) & 0x01)
#define ENTRY_LT(entry)    (((entry).bytes >> 14) & 0x01)
#define ENTRY_RT(entry)    (((entry).bytes >> 15) & 0x01)
#define ENTRY_LTRIG(entry) (((entry).bytes >> 16) & 0x0f)
#define ENTRY_RTRIG(entry) (((entry).bytes >> 20) & 0x0f)
#define ENTRY_ID(entry)    (((entry).bytes >> 24) & 0xff)

void print_packet(struct entry_t e)
{
	printf("#%u - left: %u right: %u up: %u down: %u x: %u y: %u a: %u b: "
	"%u bk: %u st: %u se: %u mo: %u z: %u w: %u lt: %u rt: %u ltrig: %u "
	"rtrig: %u\n", ENTRY_ID(e), ENTRY_LEFT(e), ENTRY_RIGHT(e), ENTRY_UP(e),
	ENTRY_DOWN(e), ENTRY_X(e), ENTRY_Y(e), ENTRY_A(e), ENTRY_B(e), ENTRY_BK(e),
	ENTRY_ST(e), ENTRY_SE(e), ENTRY_MO(e), ENTRY_Z(e), ENTRY_W(e), ENTRY_LT(e),
	ENTRY_RT(e), ENTRY_LTRIG(e), ENTRY_RTRIG(e));
}

#define bail(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

int main(int argc, char **argv)
{
	if (argc != 2)
		bail("No Argument Specified\n");

	char *endptr;
	ssize_t packets = strtol(argv[1], &endptr, 10);
	if (packets < 0 || *endptr || endptr == argv[1])
		bail("Invalid Argument\n");

	int fd = open("controller0", O_RDWR);
	if (fd < 0)
		bail("Controller Does Not Exist\n");

	if (write(fd, argv[1], strlen(argv[1])) != (int) strlen(argv[1]))
		goto out;

	for (ssize_t i = 0; i < packets; i++) {
		struct entry_t entry = {0};
		ssize_t n = read(fd, &entry, sizeof(entry));
		if (n < 0 || n != sizeof(entry))
			break;

		print_packet(entry);
	}

out:
	close(fd);
	return 0;
}
