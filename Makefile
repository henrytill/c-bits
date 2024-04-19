.POSIX:
.SUFFIXES:

CC = cc
CXX = c++
CFLAGS = -std=gnu11 -Wall -Wextra -Wconversion -Wsign-conversion -g
CXXFLAGS = -std=c++20 -Wall -Wextra -Wno-exceptions -fno-rtti -g
LDFLAGS =

CURLING_CFLAGS = $(CFLAGS)
CURLING_LDFLAGS = $(LDFLAGS)

WINDOW_CFLAGS = $(CFLAGS)
WINDOW_LDFLAGS = $(LDFLAGS)

PYTHON3 = python3
PYTHON3_CFLAGS =
PYTHON3_LDFLAGS =

-include config.mk

SOURCES =\
	base64.c \
	curling.c \
	demo_oop.c \
	fnv.c \
	fnv_test.c \
	hashtable.c \
	hashtable_test.c \
	hashtable_wrapper.c \
	overflow.c \
	poll.c \
	threadtest.c \
	window.c

BIN =\
	base64 \
	curling \
	demo_oop \
	fnv_test \
	hashtable_test \
	overflow \
	poll \
	threadtest \
	window

OBJ =\
	fnv.o \
	hashtable.o \
	hashtable_wrapper.o

.PHONY: all
all: $(BIN) hashtable.so

base64: base64.c
	$(CC) $(CFLAGS) $(.ALLSRC) $(LDFLAGS) -lcrypto -o $@

curling: curling.c
	$(CC) $(CURLING_CFLAGS) $(.ALLSRC) $(CURLING_LDFLAGS) -lcurl -o $@

fnv.o: fnv.c fnv.h
	$(CC) $(CFLAGS) -DNDEBUG -o $@ -c $<

fnv_test: fnv_test.c fnv.o

hashtable.o: hashtable.c hashtable.h
	$(CC) $(CFLAGS) -DNDEBUG -o $@ -c $<

hashtable_wrapper.o: hashtable_wrapper.c
	$(CC) $(PYTHON3_CFLAGS) -fPIC -DNDEBUG -o $@ -c $<

hashtable.so: fnv.o hashtable.o hashtable_wrapper.o
	$(CC) -shared $(PYTHON3_LDFLAGS) -o $@ $(.ALLSRC)

hashtable_test: hashtable_test.c hashtable.o fnv.o

overflow: overflow.c
	$(CC) $(CFLAGS) -ftrapv $(.ALLSRC) $(LDFLAGS) -o $@

threadtest: threadtest.c
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -pthread $(.ALLSRC) $(LDFLAGS) -o $@

window: window.c
	$(CC) $(WINDOW_CFLAGS) $(.ALLSRC) $(WINDOW_LDFLAGS) -lX11 -lGL -lGLU -lGLEW -o $@

hello: hello.cpp
	$(CXX) $(CXXFLAGS) -D SAY_GOODBYE $(.ALLSRC) -o $@

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.SUFFIXES: .c
.c:
	$(CC) $(CFLAGS) $(.ALLSRC) $(LDFLAGS) -o $@

.PHONY: check test
check test: $(BIN) hashtable.so
	./base64
	./curling
	./fnv_test
	./threadtest foo bar baz
	./hashtable_test
	$(PYTHON3) hashtable_test.py

.PHONY: lint
lint:
	clang-tidy --quiet -p compile_commands.json $(SOURCES)

.PHONY: clean
clean:
	rm -f $(BIN) $(OBJ)
	rm -f hashtable.so
	rm -rf build
	rm -rf zig-out

.PHONY: distclean
distclean: clean
	rm -rf zig-cache

# Local Variables:
# mode: makefile-bsdmake
# End:
