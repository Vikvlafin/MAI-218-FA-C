#!/bin/sh
set -eu
cd "$(dirname "$0")"
out=${1:?Usage: sh build.sh OUTPUT_DIRECTORY [san]}
mkdir -p "$out"
out=$(cd "$out" && pwd)
flags='-std=c11 -Wall -Wextra -Wpedantic -Werror -g'
if [ "${2:-}" = san ]; then
    flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie"
fi
for dir in task_??; do
    [ -f "$dir/main.c" ] || continue
    gcc $flags "$dir/main.c" "$dir/functions.c" -lm -o "$out/$dir"
done
