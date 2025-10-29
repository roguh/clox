######### THANK YOU exDM69 from reddit (2014/7/7)
CC=gcc  # tcc, gcc, clang, etc
CFLAGS = -std=c99
CFLAGS += -Wall -Wpedantic -O3 -g -Wno-unused-function
CFLAGS += -Werror=switch # Exhaustive enums if no default
CFLAGS += -MMD   # dependency generation magic
LDLIBS = -lm
LDFLAGS =
CHONK=tests/chonkfuzz.lox

SRCS=$(wildcard *.c src/*.c src/test/*.c)
OBJS=$(SRCS:.c=.o)
DEPS=$(OBJS:.o=.d)

.PHONY: all clean

all: main

commitready: fmt lint unit integration

main: $(OBJS)

-include $(DEPS)  # -MMD generates small Makefiles .d that contain the dependencies of each object

clean:
	rm -f vgcore*
	rm -f $(DEPS) $(OBJS)
	rm -f $(CHONK)

cloc: clean
	cloc .

memory-check: main
	valgrind --quiet --show-leak-kinds=all --leak-check=full ./main --debug --tests
	./tests/runner.sh 'valgrind --show-leak-kinds=all --leak-check=full --quiet ./main'

memory-fuzz: main
	./tests/fuzz.py > $(CHONK)
	valgrind --quiet --show-leak-kinds=all --leak-check=full ./main $(CHONK) > /dev/null

fuzz: main
	./tests/fuzz.py > $(CHONK)
	./main $(CHONK) > /dev/null

integration: main
	./tests/runner.sh ./main

unit: main
	./main --tests

fmt:
lint: # Hopefully -Wall is good enough for now

security-analysis:
