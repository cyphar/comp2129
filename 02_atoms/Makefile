# Copyright (C) 2017 Aleksa Sarai <cyphar@cyphar.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

NAME=atoms

CC ?= clang
SANFLAGS=-fsanitize=address
CFLAGS=-std=c99 -Wall -Wextra -Werror -Wno-error=unused-parameter -ggdb -O0
LDFLAGS=

SRC=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJS=$(patsubst %.c,%.o,$(SRC))

TESTS=$(shell find tests/* -type d)

.PHONY: test clean

$(NAME): $(OBJS)
	$(CC) $(SANFLAGS) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(SANFLAGS) -c -o $@ $<

test: $(NAME)
	for test in $(TESTS); do \
		ATOMS=$(PWD)/$(NAME) ./tests/run.sh $$test; \
	done

clean:
	rm -f $(OBJS) $(NAME)
