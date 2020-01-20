COMPILE = gcc -c -std=c99
LINK = gcc

CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Iinclude
CFLAGS += -DDATA=extern

CFILES =
CFILES += src/api.c
CFILES += src/ast.c
CFILES += src/backtrace_linux.c
CFILES += src/data.c
CFILES += src/parse.c
CFILES += src/process.c
CFILES += src/write_c_interface.c
CFILES += src/logging.c
CFILES += src/main.c
CFILES += src/memoryalloc.c

LDFLAGS += -rdynamic # needed so that backtrace() can print function names

OBJECTS = $(CFILES:%.c=BUILD/%.o)

all: glsl-processor

clean:
	rm -rf BUILD glsl-processor

BUILD/%.o: %.c BUILD/src
	$(COMPILE) $(CFLAGS) -o $@ $<

BUILD/src:
	mkdir -p BUILD/src

glsl-processor: $(OBJECTS)
	$(LINK) $(LDFLAGS) $^ -o $@
