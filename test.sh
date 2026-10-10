#!/bin/sh
set -eu
suffix=''
case "${1:-plain}" in
    plain) ;;
    san) suffix='_san' ;;
    *) printf 'Usage: sh test.sh [plain|san]\n' >&2; exit 1 ;;
esac
for script in lab_01/task_*/test.sh; do
    dir=${script%/test.sh}
    task=${dir##*/}
    sh "$script" "./build/$task$suffix" "./build/${task}_test$suffix"
done
sh test_stdout.sh "$suffix"
