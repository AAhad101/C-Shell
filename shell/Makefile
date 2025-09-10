CC=gcc
CFLAGS= -Iinclude -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm
SRC=src/main.c src/prompt.c src/parser.c src/executor.c src/hop.c src/reveal.c src/arbitrary.c src/log.c src/background.c src/general.c src/activities.c src/signals.c 

# Object files go to build/ folder, replacing src/ and .c with build/ and .o
OBJ=$(patsubst src/%.c,build/%.o,$(SRC))

all: shell.out

shell.out: $(OBJ)
	$(CC) -o $@ $(OBJ)

# Rule to compile .c file from src/ to .o in build/
build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build shell.out log.txt

