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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define bail(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

struct image_header_t {
	uint32_t width;
	uint32_t height;
	uint16_t magic;
} __attribute__((packed));

struct pixel_t {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t unused;
};

int main(int argc, char **argv)
{
	if (argc != 2)
		bail("No Filename Specified\n");
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		bail("File Does Not Exist\n");

	/* Read header */
	struct image_header_t hdr = {0};
	int n = read(fd, &hdr, sizeof(hdr));
	if (n != sizeof(hdr))
		bail("Invalid Image Header\n");
	if (hdr.magic != 60535)
		bail("Invalid Image Header\n");

	/* Create pixel array. Indexing is <width>*y + x. */
	int *grid = malloc(hdr.width * hdr.height * sizeof(int));
	if (!grid)
		bail("Cannot Allocate\n");

	/* Indexed colours. */
	size_t index_length = 0;
	uint32_t *index = NULL;

	for (size_t y = 0; y < hdr.height; y++) {
		for (size_t x = 0; x < hdr.width; x++) {
			/* Read pixel value. */
			struct pixel_t pixel = {0};
			int n = read(fd, &pixel, sizeof(pixel));
			if (n != sizeof(pixel))
				goto err_invalid_data;
			if (pixel.unused != 0)
				goto err_invalid_data;

			/* Find offset in index. */
			uint32_t colour = (pixel.red << 16) | (pixel.green << 8) | (pixel.blue << 0);
			ssize_t index_idx = -1;
			for (size_t i = 0; i < index_length; i++)
				if (index[i] == colour)
					index_idx = i;
			/* Need to add a new entry in the index. */
			if (index_idx < 0) {
				index = realloc(index, ++index_length * sizeof(*index));
				index_idx = index_length - 1;
				index[index_idx] = colour;
			}

			grid[hdr.width * y + x] = index_idx;
		}
	}

	for (size_t y = 0; y < hdr.height; y++) {
		printf("[");
		for (size_t x = 0; x < hdr.width; x++) {
			char *comma = ",";
			if (x == hdr.width - 1)
				comma = "";
			printf(" %d%s", grid[hdr.width * y + x], comma);
		}
		printf(" ]\n");
	}

	free(grid);
	free(index);
	return 0;

err_invalid_data:
	free(grid);
	free(index);
	bail("Invalid Image Data\n");
}
