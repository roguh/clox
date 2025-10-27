######### THANK YOU exDM69 from reddit (2014/7/7)
CC:=clang
CFLAGS = -std=c99
CFLAGS += -Wall -Wpedantic -O2 -g -Wno-unused-function
CFLAGS += -Wswitch # Exhaustive enums if no default
CFLAGS += -MMD   # dependency generation magic
LDLIBS = -lm
LDFLAGS =

SRCS=$(wildcard *.c src/*.c src/test/*.c)
OBJS=$(SRCS:.c=.o)
DEPS=$(OBJS:.o=.d)

.PHONY: all clean

all: main

commitready: fmt lint unit integration

main: $(OBJS)

-include $(DEPS)  # this makes magic happen

clean:
	rm -f vgcore*
	rm -f $(DEPS) $(OBJS)

cloc: clean
	cloc .

# Run with file tests
memory-check: main
	valgrind --show-leak-kinds=all --leak-check=full ./main --debug --tests

integration: main
	./test.sh ./main

unit: main
	./main --tests

fmt:
lint:
fuzz:
security-analysis:
