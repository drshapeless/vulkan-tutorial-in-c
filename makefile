include config.mk

SRC = main.c
OBJ = $(SRC:.c=.o)

all: sample

main.o: config.h

sample: $(OBJ) shaders
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

shaders:
	glslc shaders/shader.vert -o shaders/vert.spv
	glslc shaders/shader.frag -o shaders/frag.spv

clean:
	rm -f sample $(OBJ)

.PHONY: all clean shaders
