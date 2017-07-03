#/bin/sh

self="$(readlink -f "$(dirname "$BASH_SOURCE")")"
exec diff $self/stage0.as $self/stage4.as
