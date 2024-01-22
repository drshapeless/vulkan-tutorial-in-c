include config.mk

SRC = main.c
OBJ = $(SRC:.c=.o)

all: sample

main.o: config.h

sample: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f sample $(OBJ)

.PHONY: all clean
