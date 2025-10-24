######### THANK YOU exDM69 from reddit (2014/7/7)
CFLAGS = -std=c99
CFLAGS += -Wall -Wpedantic -O2 -g
CFLAGS += -MMD   # dependency generation magic
CFLAGS += -DDEBUG_TRACE=$(DEBUG_TRACE)
LDLIBS = -lm
LDFLAGS =

SRCS=$(wildcard *.c) $(wildcard test/*.c)
OBJS=$(SRCS:.c=.o)
DEPS=$(OBJS:.o=.d)

.PHONY: all clean

all: main

debug:
	make DEBUG_TRACE=true

main: $(OBJS)

-include $(DEPS)  # this makes magic happen

clean:
	rm -f $(DEPS) $(OBJS)

fmt:
lint:
test:
test-integration:
test-unit:
test-fuzz:
security-analysis:
