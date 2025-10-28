######### THANK YOU exDM69 from reddit (2014/7/7)
CFLAGS = -std=c99
CFLAGS += -Wall -Wpedantic -O2 -g -Wno-unused-function
CFLAGS += -Werror=switch # Exhaustive enums if no default
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

-include $(DEPS)  # -MMD generates small Makefiles .d that contain the dependencies of each object

clean:
	rm -f vgcore*
	rm -f $(DEPS) $(OBJS)

cloc: clean
	cloc .

memory-check: main
	valgrind --quiet --show-leak-kinds=all --leak-check=full ./main --debug --tests
	./test.sh 'valgrind --show-leak-kinds=all --leak-check=full --quiet ./main'

integration: main
	./test.sh ./main

unit: main
	./main --tests

fmt:
lint:
fuzz:
security-analysis:
