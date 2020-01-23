COMPILE = gcc -c -std=c99
LINK = gcc

CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Iinclude

CFILES =
CFILES += src/builder.c
CFILES += src/data.c
CFILES += src/parse.c
CFILES += src/logging.c
CFILES += src/memory.c

OBJECTS = $(CFILES:%.c=BUILD/%.o)

all: glsl-processor.a

clean:
	rm -rf BUILD glsl-processor.lib example

BUILD/%.o: %.c BUILD/src
	$(COMPILE) $(CFLAGS) -o $@ $<

BUILD/src:
	mkdir -p BUILD/src BUILD/example

glsl-processor.a: $(OBJECTS)
	$(AR) rcs $@ $^

example: example.c glsl-processor.a
	$(CC) $(CFLAGS) -o $@ $^
