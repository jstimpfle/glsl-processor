COMPILE = gcc -c -std=c99
LINK = gcc

CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Iinclude
CFLAGS += -DDATA=extern

CFILES =
CFILES += src/ast.c
CFILES += src/builder.c
CFILES += src/data.c
CFILES += src/parse.c
CFILES += src/process.c
CFILES += src/logging.c
CFILES += src/memoryalloc.c

CFILES += example/main.c
CFILES += example/write_c_interface.c

OBJECTS = $(CFILES:%.c=BUILD/%.o)

all: glsl-processor example

clean:
	rm -rf BUILD glsl-processor

BUILD/%.o: %.c BUILD/src
	$(COMPILE) $(CFLAGS) -o $@ $<

BUILD/src:
	mkdir -p BUILD/src BUILD/example

glsl-processor: $(OBJECTS)
	$(LINK) $(LDFLAGS) $^ -o $@
