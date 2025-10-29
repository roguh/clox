#!/usr/bin/env bash
set -eo pipefail

i=0
BIN="${1:-./main}"

echo "==== TOKENIZER TESTS ===="
for f in $(ls test_files/lex/*.lox); do
    echo "== TEST: $f =="
    name="${f%.*}"
    $BIN --lex "$(cat $f)" > "$name.out"
    diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
    i="$((i + 1))"
done

echo
echo
echo "==== DISASSEMBLY TESTS ===="
for f in $(ls test_files/disassemble/*.lox); do
    echo "== TEST: $f =="
    name="${f%.*}"
    $BIN --dis "$(cat $f)" > "$name.out"
    diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
    i="$((i + 1))"
done

echo
echo
echo "==== EXECUTION TESTS ===="
for f in $(ls test_files/eval/*.lox); do
    echo "== TEST: $f =="
    name="${f%.*}"
    $BIN "$f" > "$name.out"
    diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
    i="$((i + 1))"
done

echo "ALL $i TESTS PASS!"
