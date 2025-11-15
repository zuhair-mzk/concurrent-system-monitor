# Define the compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -std=c99 -Werror

# Define the target executable name
TARGET = sys_stats

# List of source files
SRCS = main.c stats_functions.c

# List of object files, replace .c from SRCS with .o
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = stats_functions.h

# Default target
.PHONY: all
all: $(TARGET)

# Link the target binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)

# Run the program
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# Display help text
.PHONY: help
help:
	@echo "Available make targets:"
	@echo "  all    - Builds the target binary ($(TARGET))"
	@echo "  clean  - Removes all build artifacts"
	@echo "  run    - Executes the compiled binary"
	@echo "  help   - Displays this help message"


