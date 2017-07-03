#!/bin/sh

self="$(readlink -f "$(dirname "$BASH_SOURCE")")"
rm -f $self/a.a
