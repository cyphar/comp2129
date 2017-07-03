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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define bail(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

/* This is not defined on modern glibc, so redefine it here. */
char *strdup(const char *str)
{
	size_t len = strlen(str);
	char *new = malloc(len * sizeof(*new));
	memcpy(new, str, len * sizeof(*new));
	return new;
}

/*
 * Gets a new line from the given file descriptor, caller responsible for
 * calling free on returned string. NULL is returned iff EOF was hit without
 * any characters being read.
 */
char *readline(const int fd)
{
	size_t len = 0;
	char *line = NULL;
	bool eof = false;

	while (true) {
		char ch;
		int nread = read(fd, &ch, sizeof(ch));
		if (!nread)
			eof = true;
		if (eof || ch == '\n')
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
	if (!strlen(line) && eof) {
		free(line);
		return NULL;
	}

	return line;
}

enum direction_t {
	NORTH,
	SOUTH,
	EAST,
	WEST,
	NUM_DIRECTION,
};

enum direction_t parse_direction(const char *direction)
{
	if (!strcmp(direction, "NORTH"))
		return NORTH;
	if (!strcmp(direction, "SOUTH"))
		return SOUTH;
	if (!strcmp(direction, "EAST"))
		return EAST;
	if (!strcmp(direction, "WEST"))
		return WEST;
	return NUM_DIRECTION;
}

struct room_t {
	char *name;
	struct room_t *directions[NUM_DIRECTION];
};

void room_display(struct room_t *room)
{
	puts("");
	puts(room->name);
	printf(" ---%c--- \n", room->directions[NORTH] ? 'N' : '-');
	printf("|       |\n");
	printf("|       |\n");
	printf("%c       %c\n", room->directions[WEST] ? 'W' : '|', room->directions[EAST] ? 'E' : '|');
	printf("|       |\n");
	printf("|       |\n");
	printf(" ---%c--- \n", room->directions[SOUTH] ? 'S' : '-');
	puts("");
}

struct dungeon_t {
	ssize_t num;
	char **names;
	struct room_t *rooms;
};

ssize_t dungeon_room(struct dungeon_t *dungeon, const char *name)
{
	for (ssize_t i = 0; i < dungeon->num; i++)
		if (!strcmp(name, dungeon->names[i]))
			return i;
	return -1;
}

void parse_dungeon_header(struct dungeon_t *dungeon, char *header)
{
	dungeon->names = NULL;
	dungeon->num = 0;

	for (char *p = header; *p != '\0'; p++) {
		/* Skip over blanks. */
		while (isspace(*p))
			p++;
		if (*p == '\0')
			break;

		/* Add to argv. */
		dungeon->names = realloc(dungeon->names, ++dungeon->num * sizeof(char *));
		dungeon->names[dungeon->num-1] = p;

		/* Null terminate. */
		while (!isspace(*p) && *p != '\0')
			p++;
		if (*p == '\0')
			break;
		*p = '\0';
	}

	/* Make copies. */
	for (ssize_t i = 0; i < dungeon->num; i++)
		dungeon->names[i] = strdup(dungeon->names[i]);
}

void parse_dungeon(struct dungeon_t *dungeon, const char *filename)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
		bail("Unknown Level File\n");

	char *header = readline(fd);
	if (!header)
		bail("Read Header Failed\n");
	parse_dungeon_header(dungeon, header);
	free(header);

	dungeon->rooms = malloc(dungeon->num * sizeof(struct room_t));
	memset(dungeon->rooms, 0, dungeon->num * sizeof(struct room_t));

	for (ssize_t i = 0; i < dungeon->num; i++)
		dungeon->rooms[i].name = dungeon->names[i];

	while (true) {
		char *line = readline(fd);
		if (!line)
			break;

		/* Parsing silly formats is _so much fun_. */
		char from_str[8192+1],
		     dir_str[8192+1],
		     to_str[8192+1];

		if (sscanf(line, "%8192s > %8192s > %8192s", from_str, dir_str, to_str) != 3)
			goto next;

		int from = dungeon_room(dungeon, from_str),
		    dir = parse_direction(dir_str),
		    to = dungeon_room(dungeon, to_str);

		if (from < 0 || to < 0 || dir == NUM_DIRECTION)
			goto next;

		dungeon->rooms[from].directions[dir] = &dungeon->rooms[to];
next:
		free(line);
	}

	close(fd);
}

int main(int argc, char **argv)
{
	struct dungeon_t dungeon = {0};

	if (argc != 2)
		bail("No Level File Specified\n");

	parse_dungeon(&dungeon, argv[1]);
	struct room_t *current = &dungeon.rooms[0];

	room_display(current);

	while (true) {
		char *line = readline(STDIN_FILENO);
		if (!line || !strcmp(line, "QUIT")) {
			free(line);
			break;
		}
		int direction = parse_direction(line);
		free(line);

		if (direction == NUM_DIRECTION)
			puts("What?");
		else if (!current->directions[direction])
			puts("No Path This Way");
		else
			current = current->directions[direction];

		room_display(current);

		if (direction == NUM_DIRECTION)
			break;
	}

	for (ssize_t i = 0; i < dungeon.num; i++)
		free(dungeon.names[i]);
	free(dungeon.names);
	free(dungeon.rooms);
	return 0;
}
