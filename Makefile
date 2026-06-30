# === Project settings ===
SRC     	= $(wildcard src/*.c) $(wildcard generated/*.c)
OBJ			= $(patsubst %.c,obj/%.o,$(notdir $(SRC)))
HDR     	= $(wildcard include/*.h)
BUILD_DIR 	= obj
GCC		 	= gcc
FLAGS    	= -Iinclude -lm
DEBUG_FLAGS = -g -DDEBUG
vpath %.c src generated

# === Rules ===

all: clean Whiteboard

debug: FLAGS += $(DEBUG_FLAGS)
debug: clean Whiteboard

obj/%.o: %.c $(HDR)
	$(GCC) $(FLAGS) -c  $< -o $@

Whiteboard: $(OBJ)
	$(GCC) $(FLAGS) obj/* -o Whiteboard
	
clean:
	rm -f ./Whiteboard
	rm -rf obj/*
