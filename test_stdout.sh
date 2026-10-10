#!/bin/sh
set -eu
suffix=${1:-}
check() {
    if "$@" >/dev/full 2>/dev/null; then
        printf 'Undetected stdout error: %s\n' "$*" >&2
        return 1
    fi
}
check "./build/task_01$suffix" 10 -a
check "./build/task_02$suffix" 0.001
check "./build/task_03$suffix" -m 12 3
check "./build/task_05$suffix" 0.001 0.5
check "./build/task_06$suffix"
check "./build/task_09$suffix" 0 0
if printf '10 1 Stop\n' | "./build/task_10$suffix" >/dev/full 2>/dev/null; then
    printf 'Undetected stdout error: task_10\n' >&2
    exit 1
fi
printf 'stdout: all seven programs report write errors\n'
