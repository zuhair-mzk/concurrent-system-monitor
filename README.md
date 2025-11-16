# Concurrent System Monitoring Tool

A real-time, multi-process system monitoring tool that reports CPU usage, memory utilization, user sessions, and system information on Linux systems. This project demonstrates advanced systems programming concepts including concurrent programming, inter-process communication (IPC) via pipes, and signal handling.

> **Note:** This project was developed as the final assignment for CSCB09 (Winter 2024) at the University of Toronto Scarborough.

## 🎯 Overview

This system monitoring tool efficiently collects and displays vital system statistics through a concurrent architecture. By spawning separate child processes for each metric type (memory, CPU, users), the program achieves non-blocking, simultaneous data collection while maintaining clean output ordering through careful synchronization.

### Key Features

- **Concurrent Data Collection**: Separate child processes gather system metrics independently
- **Inter-Process Communication**: Pipes facilitate data transfer from child to parent processes
- **Signal Handling**: Graceful handling of `SIGINT` (Ctrl-C) and `SIGTSTP` (Ctrl-Z)
- **Flexible Display Modes**: Sequential or refreshing display with optional graphical representations
- **Configurable Sampling**: User-defined sample count and time delay between samples

## 🖥️ System Requirements

**This program is designed exclusively for Linux systems** and requires:

- **Operating System**: Linux (tested on Ubuntu/Debian-based distributions)
- **Compiler**: GCC with C99 standard support
- **Required Headers**: 
  - `sys/sysinfo.h` (Linux-specific)
  - `sys/utsname.h`
  - `sys/resource.h`
  - `utmp.h`
  - Standard POSIX headers

### Why Linux-Only?

The program relies on Linux-specific system interfaces:
- `/proc/stat` for CPU statistics
- `/proc/uptime` for system uptime
- `sys/sysinfo.h` for memory statistics (not available on macOS/Windows)
- `utmp.h` for user session information

## 🚀 How to Run

### On Linux (Direct)

```bash
make
./sys_stats
./sys_stats --samples=5 --tdelay=2 --graphics
```

### Using Docker (Any Platform)

```bash
docker build -t sys-stats .
docker run --rm -it sys-stats
docker run --rm -it sys-stats ./sys_stats --graphics --samples=5
```

### Other Options
- **SSH to Linux server**: Compile and run on remote Linux machine
- **Virtual Machine**: Run Ubuntu in VirtualBox/VMware and compile there

## 📖 Command Line Arguments

The program supports various command-line options to customize its behavior:

### Flags

- `--system` or `-s`: Display only system usage statistics (memory, CPU)
- `--user` or `-u`: Display only user session information
- `--graphics` or `-g`: Include graphical output for memory and CPU usage
- `--sequential` or `-q`: Output sequentially without screen refresh (useful for redirecting to files)
- `--samples=N` or `-n N`: Number of samples to collect (default: 10)
- `--tdelay=T` or `-t T`: Delay in seconds between samples (default: 1)

### Positional Arguments

You can also provide samples and delay as positional arguments:

```bash
./sys_stats [samples] [tdelay]
```

### Examples

```bash
# Default: 10 samples, 1 second delay, all stats
./sys_stats

# Custom samples and delay
./sys_stats --samples=5 --tdelay=2

# System stats only with graphics
./sys_stats --system --graphics

# User stats only
./sys_stats --user

# Sequential output (good for piping to file)
./sys_stats --sequential > output.txt

# Positional arguments: 20 samples, 3 second delay
./sys_stats 20 3

# Mixed flags and positional
./sys_stats --graphics 15 2
```

## 🏗️ Architecture & Design

### Problem-Solving Approach

The program employs a **multi-process concurrent architecture** to efficiently gather system statistics:

1. **Main Process**: Orchestrates the sampling loop, manages child processes, and formats output
2. **Memory Process**: Child process that gathers memory statistics and sends via pipe
3. **User Process**: Child process that reads user sessions from `/var/run/utmp` and sends via pipe
4. **CPU Process**: Child process that calculates CPU usage and sends via pipe

### Concurrency Strategy

```
┌─────────────────────────────────────────────────────────┐
│                     Main Process                        │
│  • Manages sampling loop                                │
│  • Spawns child processes                               │
│  • Reads from pipes                                     │
│  • Formats and displays output                          │
└─────────────────────────────────────────────────────────┘
         │                    │                    │
         │ fork()             │ fork()             │ fork()
         ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  Memory Child   │  │   User Child    │  │   CPU Child     │
│                 │  │                 │  │                 │
│ • sysinfo()     │  │ • getutent()    │  │ • /proc/stat    │
│ • Calculate GB  │  │ • Parse users   │  │ • CPU usage %   │
│ • Write to pipe │  │ • Write to pipe │  │ • Write to pipe │
└─────────────────┘  └─────────────────┘  └─────────────────┘
```

### Inter-Process Communication

Each child process uses a dedicated pipe to send data back to the parent:

```c
int mem_pipe[2], cpu_pipe[2], user_pipe[2];
pipe(mem_pipe);  // Create pipe: [0]=read end, [1]=write end

pid_t pid = fork();
if (pid == 0) {
    // Child: close read end, write data, close write end, exit
    close(mem_pipe[0]);
    write(mem_pipe[1], buffer, size);
    close(mem_pipe[1]);
    exit(0);
} else {
    // Parent: close write end, read data, close read end, wait for child
    close(mem_pipe[1]);
    read(mem_pipe[0], buffer, size);
    close(mem_pipe[0]);
    wait(NULL);
}
```

### Signal Handling

The program implements robust signal handling:

- **SIGTSTP (Ctrl-Z)**: Ignored to prevent background suspension during interactive use
- **SIGINT (Ctrl-C)**: Triggers a confirmation prompt asking if the user wants to quit

```c
signal(SIGTSTP, SIG_IGN);           // Ignore Ctrl-Z
signal(SIGINT, sigint_handler);      // Handle Ctrl-C with prompt
```

## 📋 Key Functions

### Main Process Functions

- **`main()`**: Entry point that orchestrates the sampling loop and process management
- **`sigint_handler(int sig_num)`**: Handles SIGINT signal with user confirmation prompt
- **`display_header()`**: Displays iteration info and memory usage of the monitoring tool itself

### Child Process Functions

These functions spawn child processes and handle pipe communication:

- **`launchMemoryStatsProcess()`**: Spawns child to gather memory statistics
  - Uses `sysinfo()` to get RAM and swap usage
  - Converts bytes to GB for readability
  - Sends formatted data via pipe
  
- **`launchUserStatsProcess()`**: Spawns child to gather user session information
  - Reads from `/var/run/utmp` using `getutent()`
  - Filters for `USER_PROCESS` entries
  - Builds linked list of users and returns to parent
  
- **`launchCpuStatsProcess()`**: Spawns child to calculate CPU usage
  - Reads `/proc/stat` for CPU time statistics
  - Calculates usage based on idle vs. total time
  - Optionally generates graphical representation

### Statistics Gathering Functions

- **`gather_memory_stats()`**: Collects memory statistics using `sysinfo()`
- **`get_cpu_idle_total_times()`**: Reads CPU times from `/proc/stat`
- **`calculate_and_print_cpu_usage()`**: Computes CPU utilization percentage
- **`get_cpu_cores()`**: Returns number of online CPU cores
- **`print_system_info()`**: Displays system information and uptime

### Display Functions

- **`display_memory_stats()`**: Formats and prints memory usage
- **`print_user_list()`**: Displays user sessions in tabular format
- **`print_cpu_graphics()`**: Shows graphical CPU usage representation
- **`append_graphical_representation()`**: Creates visual memory usage changes

### Utility Functions

- **`parse_arguments()`**: Parses command-line arguments and flags
- **`append_user()`**: Adds user to linked list
- **`free_user_list()`**: Frees memory allocated for user list
- **`update_cpu_graphics()`**: Updates CPU usage graphical bars

## 📊 Output Format

### Standard Output

```
Nbr of samples: 10 -- every 1 secs
 Memory usage: 4092 kilobytes
---------------------------------------
### Memory ### (Phys.Used/Tot -- Virtual Used/Tot) 
9.78 GB / 15.37 GB  -- 9.78 GB / 16.33 GB
9.77 GB / 15.37 GB  -- 9.77 GB / 16.33 GB
...
---------------------------------------
### Sessions/users ### 
 john       pts/0 (192.168.1.100)
 jane       tty7  (:0)
---------------------------------------
Number of cores: 4 
 total cpu use = 12.50%
---------------------------------------
### System Information ### 
 System Name = Linux
 Machine Name = hostname
 Version = #99-Ubuntu SMP Thu Sep 23 17:29:00 UTC 2021
 Release = 5.4.0-88-generic
 Architecture = x86_64
 System running since last reboot: 3 days 19:27:30 (91:27:30)
---------------------------------------
```

### With Graphics Flag

When `--graphics` is enabled:

**Memory Graphics:**
```
9.75 GB / 15.37 GB  -- 9.75 GB / 16.33 GB   |o 0.00 (9.75) 
9.85 GB / 15.37 GB  -- 9.85 GB / 16.33 GB   |#########* 0.09 (9.85) 
10.06 GB / 15.37 GB -- 10.06 GB / 16.33 GB  |####################* 0.20 (10.06)
```

- `#` = positive memory change
- `:` = negative memory change  
- `*` = end marker for significant change
- `o` = minimal/no change

**CPU Graphics:**
```
 total cpu use = 15.57%
         ||| 0.25 
         ||||||||| 6.93 
         |||||||||||||||| 13.83 
```

Each `|` represents ~1% CPU usage.

## 🔧 Compilation

### Using Make

```bash
# Build the executable
make

# Clean build artifacts
make clean

# Rebuild from scratch
make clean && make

# Run after building
make run
```

### Manual Compilation

```bash
# Compile with all warnings and debugging symbols
gcc -Wall -g -std=c99 -Werror -c main.c -o main.o
gcc -Wall -g -std=c99 -Werror -c stats_functions.c -o stats_functions.o
gcc -Wall -g -std=c99 -Werror -o sys_stats main.o stats_functions.o

# Run
./sys_stats
```

### Makefile Structure

The Makefile follows best practices:

- **Macros**: `CC`, `CFLAGS` for easy configuration
- **Intermediate Objects**: Generates `.o` files before linking
- **Clean Target**: Removes all build artifacts
- **Phony Targets**: `.PHONY` declarations for non-file targets

## 🧪 Testing

### Basic Tests

```bash
# Test default behavior
./sys_stats

# Test with different sample counts
./sys_stats --samples=3
./sys_stats 3

# Test with different delays
./sys_stats --tdelay=5
./sys_stats 10 5

# Test graphics mode
./sys_stats --graphics

# Test system-only mode
./sys_stats --system

# Test user-only mode
./sys_stats --user

# Test sequential output
./sys_stats --sequential

# Test signal handling (try Ctrl-C during execution)
./sys_stats --samples=20 --tdelay=2
```

### Edge Cases

```bash
# Large number of samples
./sys_stats 100 1

# Long delay
./sys_stats 5 10

# Combined flags
./sys_stats --system --graphics --sequential --samples=5

# Output to file
./sys_stats --sequential --samples=5 > output.txt
```

## 📝 Code Quality

### Modularity

- **main.c**: Program entry point and orchestration
- **stats_functions.c**: Implementation of all statistics gathering and display functions
- **stats_functions.h**: Function declarations and type definitions

### Best Practices

✅ No global variables (all state passed via parameters)  
✅ Comprehensive error checking with `perror()` and exit codes  
✅ Modular functions with clear, single responsibilities  
✅ Detailed comments and documentation  
✅ Proper memory management (malloc/free for linked lists)  
✅ Clean separation of concerns  
✅ POSIX-compliant system calls (no shell commands executed)

## 🎓 Educational Value

This project demonstrates proficiency in:

- **Systems Programming**: Direct interaction with Linux kernel interfaces
- **Concurrent Programming**: Multi-process architecture with fork()
- **Inter-Process Communication**: Pipe-based data exchange
- **Signal Handling**: Graceful interrupt management
- **Memory Management**: Dynamic allocation, linked lists, proper cleanup
- **File I/O**: Reading from `/proc` filesystem and `utmp`
- **Build Automation**: Professional Makefile with proper dependencies
- **Code Organization**: Clean modular architecture

## 🐛 Known Limitations

- **Linux-Only**: Will not compile or run on macOS or Windows
- **Root Access**: Some statistics may require elevated privileges
- **Terminal Dependency**: Best viewed in a standard terminal (80+ columns)
- **Concurrent Output**: Very fast sampling (tdelay < 1s) may cause minor output ordering issues

## 📚 References

- [Linux Programmer's Manual - sysinfo(2)](https://man7.org/linux/man-pages/man2/sysinfo.2.html)
- [Linux Programmer's Manual - uname(2)](https://man7.org/linux/man-pages/man2/uname.2.html)
- [Linux Programmer's Manual - utmp(5)](https://man7.org/linux/man-pages/man5/utmp.5.html)
- [Linux Kernel Documentation - /proc/stat](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
- [Advanced Programming in the UNIX Environment by W. Richard Stevens](https://www.apuebook.com/)

- [Linux Kernel Documentation - /proc/stat](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
- [Advanced Programming in the UNIX Environment by W. Richard Stevens](https://www.apuebook.com/)
