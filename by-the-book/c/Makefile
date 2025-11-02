######### THANK YOU exDM69 from reddit (2014/7/7)
CC=gcc                  # Alternatives: tcc, gcc, clang, etc
CFLAGS:=-std=c99
# CFLAGS+=-O3             # Performance!
CFLAGS+=-Wall -Wpedantic
CFLAGS+=-g              # Debugging symbols
CFLAGS+=-Werror=switch  # Exhaustive enums if no default
CFLAGS+=-MMD            # -MMD generates small Makefiles with file extension .d that contain the dependencies of each object
LDLIBS:=-lm
LDFLAGS:=
VAL:=valgrind --quiet --show-leak-kinds=all --leak-check=full
SRCS:=$(wildcard *.c src/*.c src/test/*.c)
OBJS:=$(SRCS:.c=.o)
DEPS:=$(OBJS:.o=.d)
FUZZY_CHONKY_FILE:=tests/chonkfuzz.lox

all: main

commitready: fmt lint unit integration

.PHONY: all clean commitready fmt lint unit integration memory-check fuzz memory-fuzz

main: $(OBJS)

-include $(DEPS)  # Relies on -MMD

clean:
	rm -f $(DEPS) $(OBJS)
	rm -f $(FUZZY_CHONKY_FILE)
	rm -f vgcore*

cloc: clean
	cloc .

memory-check: main
	$(VAL) ./main --debug --tests
	./tests/runner.sh "$(VAL) ./main"

memory-fuzz: main
	./tests/fuzz.py > $(FUZZY_CHONKY_FILE)
	time $(VAL) ./main $(FUZZY_CHONKY_FILE) > /dev/null

fuzz: main
	./tests/fuzz.py > $(FUZZY_CHONKY_FILE)
	time ./main $(FUZZY_CHONKY_FILE) > /dev/null

integration: main
	./tests/runner.sh ./main

unit: main
	./main --tests

fmt:
lint:
security-analysis:
