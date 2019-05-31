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

#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define bail(...) \
	do { fprintf(stderr, __VA_ARGS__); exit(1); } while (0)

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

struct record_t {
	char *line;
	size_t num_fields;
	char **fields;
};

struct sortarg_t {
	int column;
	bool ascending;
};

int record_cmp(const void *left_ptr, const void *right_ptr, void *arg_ptr)
{
	struct sortarg_t arg = * (struct sortarg_t *) arg_ptr;
	struct record_t left_record = * (struct record_t *) left_ptr;
	struct record_t right_record = * (struct record_t *) right_ptr;

	/* It's a bit late to bubble up errors. */
	if (left_record.num_fields <= (size_t) arg.column || right_record.num_fields <= (size_t) arg.column)
		bail("Invalid Column\n");

	char *left = left_record.fields[arg.column];
	char *right = right_record.fields[arg.column];

	int cmp = strcmp(left, right);
	if (arg.column == 1)
		cmp = atoi(left) - atoi(right);

	return (arg.ascending ? 1 : -1) * cmp;
}

struct album_t {
	size_t num_records;
	struct record_t *records;
};

void parse_album(struct album_t *album, const char *filename)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
		bail("File Does Not Exist\n");

	/* Read in the lines. */
	while (true) {
		char *line = readline(fd);
		if (!line)
			break;
		album->records = realloc(album->records, ++album->num_records * sizeof(*album->records));
		album->records[album->num_records-1].line = line;
	}

	/* Parse the records. */
	for (size_t i = 0; i < album->num_records; i++) {
		struct record_t *record = &album->records[i];
		record->fields = NULL;
		record->num_fields = 0;

		char *p = strtok(record->line, ",");
		do {
			record->fields = realloc(record->fields, ++record->num_fields * sizeof(*record->fields));
			record->fields[record->num_fields-1] = p;
		} while ((p = strtok(NULL, ",")));
	}

	close(fd);
}

void display_album(struct album_t *album)
{
	for (size_t i = 0; i < album->num_records; i++) {
		struct record_t *record = &album->records[i];
		for (size_t j = 0; j < record->num_fields; j++)
			printf("%s%s", record->fields[j], j == record->num_fields - 1 ? "\n" : ", ");
		fflush(stdout);
	}
}

void sort_album(struct album_t *album, struct sortarg_t arg)
{
	qsort_r(album->records, album->num_records, sizeof(*album->records), record_cmp, &arg);
}

void free_album(struct album_t *album)
{
	for (size_t i = 0; i < album->num_records; i++) {
		free(album->records[i].line);
		free(album->records[i].fields);
	}
	free(album->records);
}


void cmd_display(struct album_t *album, int argc, char **argv)
{
	if (argc != 1)
		return;

	display_album(album);
}

void cmd_sort(struct album_t *album, int argc, char **argv)
{
	if (argc < 2 || argc > 3)
		return;

	struct sortarg_t arg = {
		.column = atoi(argv[1]),
		.ascending = true,
	};

	if (argc == 3)
		arg.ascending = !!strcmp(argv[2], "DESC");

	sort_album(album, arg);
}


/*
 * Parses the provided line and fills the command data. @cmd must have already
 * been allocated with cmd_alloc. Return value is < 0 if an error occurred.
 * @line will be modified by this function.
 */
int cmd_parse(int *argc, char ***argv, char *line)
{
	/* Reset to defaults. */
	int _argc = 0;
	char **_argv = NULL;

	for (char *p = line; *p != '\0'; p++) {
		/* Skip over blanks. */
		while (isspace(*p))
			p++;
		if (*p == '\0')
			break;

		/* Add to argv. */
		_argv = realloc(_argv, ++_argc * sizeof(char *));
		_argv[_argc-1] = p;

		/* Null terminate. */
		while (!isspace(*p) && *p != '\0')
			p++;
		if (*p == '\0')
			break;
		*p = '\0';
	}

	/* Need at least one argc. */
	if (_argc < 1) {
		free(_argv);
		return -1;
	}

	*argv = _argv;
	*argc = _argc;
	return 0;
}

int main(int argc, char **argv)
{
	struct album_t album = {0};
	if (argc != 2)
		bail("No File Specified\n");
	parse_album(&album, argv[1]);

	while (true) {
		int argc;
		char **argv = NULL, *line = NULL;
		bool cont = false;

		line = readline(STDIN_FILENO);
		if (!line)
			goto out;

		if (cmd_parse(&argc, &argv, line) < 0)
			goto next;

		if (!strcmp(argv[0], "QUIT"))
			goto out;
		else if (!strcmp(argv[0], "DISPLAY"))
			cmd_display(&album, argc, argv);
		else if (!strcmp(argv[0], "SORT"))
			cmd_sort(&album, argc, argv);

next:
		cont = true;
out:
		free(line);
		free(argv);
		if (!cont)
			break;
	}

	free_album(&album);
	return 0;
}
