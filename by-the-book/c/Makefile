######### THANK YOU exDM69 from reddit (2014/7/7)
CFLAGS = -std=c99
CFLAGS += -Wall -Wpedantic -O2 -g
CFLAGS += -Wswitch # Exhaustive enums if no default
CFLAGS += -MMD   # dependency generation magic
LDLIBS = -lm
LDFLAGS =

SRCS=$(wildcard *.c) $(wildcard test/*.c)
OBJS=$(SRCS:.c=.o)
DEPS=$(OBJS:.o=.d)

.PHONY: all clean

all: main

main: $(OBJS)

-include $(DEPS)  # this makes magic happen

clean:
	rm -f $(DEPS) $(OBJS)

cloc: clean
	cloc .

fmt:
lint:
test:
test-integration:
test-unit:
test-fuzz:
security-analysis:
