FROM ubuntu:22.04

# Install build essentials
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY main.c stats_functions.c stats_functions.h Makefile ./

# Build the program
RUN make

# Default command - run with default parameters
CMD ["./sys_stats"]

# To run with custom parameters:
# docker build -t sys-stats .
# docker run --rm -it sys-stats
# docker run --rm -it sys-stats ./sys_stats --graphics --samples=5
