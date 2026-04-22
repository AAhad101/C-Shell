# Makefile for the S.H.A.M. Reliable UDP Project

# Compiler and flags
CC = gcc
# CFLAGS for warnings, debugging symbols, and standard C11
CFLAGS = -Wall -Wextra -g -std=c11
# Linker flags, specifically for the OpenSSL crypto library
LDFLAGS = -lcrypto

# Source files
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Object files
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

# Executable names
CLIENT_EXE = client
SERVER_EXE = server

# Default target
all: $(CLIENT_EXE) $(SERVER_EXE)

# Rule to build the client
$(CLIENT_EXE): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build the server
$(SERVER_EXE): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile .c files to .o files
%.o: %.c sham.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts and log files
clean:
	rm -f $(CLIENT_EXE) $(SERVER_EXE) $(CLIENT_OBJ) $(SERVER_OBJ) server_log.txt client_log.txt

.PHONY: all clean
