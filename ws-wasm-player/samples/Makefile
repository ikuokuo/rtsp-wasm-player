.DEFAULT_GOAL := all

.PHONY: all
all: hello_sdl

hello_sdl: hello_sdl.cc
	@emcc -s WASM=0 $< -o $@.html
