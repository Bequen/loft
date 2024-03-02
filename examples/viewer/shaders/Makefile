FILES=$(wildcard ./*.glsl)

SPIRV=$(FILES:%.glsl=%.spirv)

debug: $(SPIRV)

%.spirv:%.glsl
	glslangValidator -g -V $^ -o $@

clean:
	rm -f ./*.spirv
