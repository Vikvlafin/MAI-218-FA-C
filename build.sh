#!/bin/sh
set -eu
flags='-std=c11 -Wall -Wextra -Wpedantic -Werror'
suffix=''
case "${1:-plain}" in
    plain) ;;
    san) flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer -g"; suffix='_san' ;;
    *) printf 'Usage: sh build.sh [plain|san]\n' >&2; exit 1 ;;
esac
mkdir -p build
for source in lab_01/task_*/main.c; do
    dir=${source%/main.c}
    task=${dir##*/}
    gcc $flags "$source" -lm -o "build/$task$suffix"
    if test -f "$dir/test.c"; then
        gcc $flags "$dir/test.c" -lm -o "build/${task}_test$suffix"
    fi
done
