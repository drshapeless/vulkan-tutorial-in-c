include config.mk

SRC = main.c
OBJ = $(SRC:.c=.o)

all: sample

main.o: config.h

sample: $(OBJ) shaders
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

shaders:
	$(MAKE) -C shaders

clean:
	rm -f sample $(OBJ)
	$(MAKE) -C shaders clean

.PHONY: all clean shaders
