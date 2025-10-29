#!/usr/bin/env bash
set -eo pipefail

BIN="${1:-./main}"

for f in $(ls test_files/lex/*.lox); do
    echo "== TEST: $f =="
    name="${f%.*}"
    $BIN --lex "$(cat $f)" > "$name.out"
    diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
done

for f in $(ls test_files/eval/*.lox); do
    echo "== TEST: $f =="
    name="${f%.*}"
    $BIN "$f" > "$name.out"
    diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
done
