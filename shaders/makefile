all: shaders

shaders: vert.spv frag.spv

vert.spv: shader.vert
	glslc shader.vert -o vert.spv

frag.spv: shader.frag
	glslc shader.frag -o frag.spv

clean:
	rm *.spv

.PHONY:
	all clean
