#!/usr/bin/env bash
set -eo pipefail

i=0
BIN="${1:-./main}"

if [ -z "$SKIP_LEX" ]; then
    echo "==== TOKENIZER TESTS ===="
    for f in $(ls tests/lex/*.lox); do
        echo "== TEST: $f =="
        name="${f%.*}"
        $BIN --lex "$(cat $f)" > "$name.out"
        diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
        i="$((i + 1))"
    done
    echo
    echo
else
    echo
fi

if [ -z "$SKIP_DIS" ]; then
    echo "==== DISASSEMBLY TESTS ===="
    for f in $(ls tests/disassemble/*.lox); do
        echo "== TEST: $f =="
        name="${f%.*}"
        $BIN --dis "$(cat $f)" > "$name.out"
        diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
        i="$((i + 1))"
    done
    echo
    echo
else
    echo
fi

if [ -z "$SKIP_EXECUTION" ]; then
    echo "==== EXECUTION TESTS ===="
    for f in $(ls tests/eval/*.lox); do
        echo "== TEST: $f =="
        name="${f%.*}"
        $BIN "$f" > "$name.out"
        diff --ignore-space-change "$name.out" "$name.expected" && echo PASS || exit 1
        i="$((i + 1))"
    done
else
    echo
fi

echo "ALL $i TESTS PASS!"
