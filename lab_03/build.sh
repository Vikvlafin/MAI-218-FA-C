#!/bin/sh
set -eu
cd "$(dirname "$0")"
out=${1:?usage: sh build.sh output_directory [san]}
mkdir -p "$out"
out=$(cd "$out" && pwd)
flags='-std=c11 -Wall -Wextra -Wpedantic -Werror -g'
if [ "${2:-}" = san ]; then
    flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie"
fi
for task in task_??; do
    gcc $flags "$task"/*.c -lm -o "$out/$task"
done

