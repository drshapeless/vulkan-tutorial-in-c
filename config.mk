# compiler
CC = clang

# includes and libs
INCS = $(shell pkg-config --cflags sdl2)
LIBS = $(shell pkg-config --libs sdl2)

# flags
CFLAGS = $(INCS) -O2 -std=c99
LDFLAGS = $(LIBS) -lvulkan -lpthread

UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
	LDFLAGS = $(LIBS) -ldl -lpthread -lvulkan
endif
