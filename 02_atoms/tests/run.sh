#!/bin/sh
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

# usage: run.sh <testdir>
#
# This script runs "stage-based" tests from a given directory. Effectively,
# each stage is an (input, expected_output) pair with a zero-indexed index that
# specifies the order in which the tests will run. $ATOMS will be run from the
# same directory as the stage files. Here is an example test directory tree
# (.in is used as input to $ATOMS and .out is the expected output):
#
# tests
#   `-- 0009_save_noclobber
#       |-- init.sh
#       |-- stage0.in
#       |-- stage0.out
#       |-- stage1.in
#       |-- stage1.out
#       |-- stage2.in
#       `-- stage2.out
#
# Before anything else, if there is an init.sh script in the root of the test
# directory it will be run *once*. Then each of the stages will be run in
# order. By default this script assumes that $ATOMS is in the parent directory
# of run.sh, but you can explicitly set the path to ATOMS by setting the
# environment variable. Here's an example usage:
#
# $ ATOMS=/path/to/atoms run.sh tests/0009_save_noclobber
# [+ ] 'tests/0009_save_noclobber'
# [ +]   -> stage 0 ... PASS
# [ +]   -> stage 1 ... PASS
# [ +]   -> stage 2 ... PASS
#
# In the case of failure, testing will be halted at that stage and a diff of
# the expected and received output will be output (in the unified diff format).

set -e

[[ "$#" == 1 ]] || { echo "usage: $0 <testdir>"; exit 1; }

self="$(readlink -f "$(dirname "$BASH_SOURCE")")"
ATOMS="${ATOMS:-$self/../atoms}"

dir="$1"
tmpdir="$(mktemp --tmpdir -d "atoms-test-$(basename $dir).XXXXXX")"

# To make things easier.
cd "$dir" &>/dev/null
[ -x "./init.sh" ] && ./init.sh

# Prefer colordiff.
diff=colordiff
if ! ("$diff" --version &>/dev/null) ; then
	diff="diff"
fi

stage=0
fail=0
echo "[+ ] '$dir'" >&2
while [ -f "stage$stage.in" ]; do
	inp="stage$stage.in"
	exp="stage$stage.out"
	out="$tmpdir/stage$stage"

	"$ATOMS" <"$inp" >"$out"
	"$diff" -u "$out" "$exp"

	if [[ "$?" == 0 ]]; then
		echo "[ +]   -> stage $stage ... PASS" >&2
	else
		echo "[ -]   -> stage $stage ... FAIL" >&2
		fail=$(($fail + 1))
	fi

	stage=$(($stage + 1))
done

if [ -x "./fini.sh" ]; then
	./fini.sh
	if [[ "$?" == 0 ]]; then
		echo "[ +]   -> final ... PASS" >&2
	else
		echo "[ -]   -> final ... FAIL" >&2
		fail=$(($fail + 1))
	fi
fi

if [[ "$fail" != 0 ]]; then
	echo "[- ] $fail failure(s)" >&2
	exit 1
fi

rm -rf "$tmpdir"
