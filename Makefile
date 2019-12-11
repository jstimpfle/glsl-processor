COMPILE = gcc -c -std=c99
LINK = gcc

CFLAGS += -Iinclude
CFLAGS += -DDATA=extern

CFILES =
CFILES += src/data.c
CFILES += src/lex.c
CFILES += src/str.c
CFILES += src/logging.c
CFILES += src/main.c
CFILES += src/memoryalloc.c

OBJECTS = $(CFILES:%.c=BUILD/%.o)

all: main

BUILD/%.o: %.c BUILD/src
	$(COMPILE) $(CFLAGS) -o $@ $<

BUILD/src:
	mkdir -p BUILD/src

main: $(OBJECTS)
	$(LINK) $(LDFLAGS) $^ -o $@
