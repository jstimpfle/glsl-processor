COMPILE = gcc -c -std=c99
LINK = gcc

CFLAGS += -g
CFLAGS += -Iinclude
CFLAGS += -DDATA=extern

CFILES =
CFILES += src/backtrace_linux.c
CFILES += src/data.c
CFILES += src/parse.c
CFILES += src/logging.c
CFILES += src/main.c
CFILES += src/memoryalloc.c

LDFLAGS += -rdynamic # needed so that backtrace() can print function names

OBJECTS = $(CFILES:%.c=BUILD/%.o)

all: main

clean:
	rm -rf BUILD main

BUILD/%.o: %.c BUILD/src
	$(COMPILE) $(CFLAGS) -o $@ $<

BUILD/src:
	mkdir -p BUILD/src

main: $(OBJECTS)
	$(LINK) $(LDFLAGS) $^ -o $@
