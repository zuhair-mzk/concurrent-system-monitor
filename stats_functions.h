// Guard to prevent double inclusion of the header file
#ifndef STATS_FUNCTIONS_H
#define STATS_FUNCTIONS_H

// Standard library and system includes for necessary functions and types
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>  // For user information
#include <getopt.h>  // For command line parsing
#include <unistd.h>
#include <sys/utsname.h>  // For system information
#include <sys/sysinfo.h>  // For system statistics like memory
#include <sys/resource.h>  // For resource usage (e.g., memory)
#include <string.h>
#include <math.h>
#include <sys/wait.h>  // For wait() in process handling
#include <signal.h>  // For signal handling

// Macro to compute the minimum of two values
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// Struct for holding memory statistics
typedef struct {
    double phys_used;  // Physical memory used
    double phys_total;  // Total physical memory
    double virt_used;  // Virtual memory used
    double virt_total;  // Total virtual memory
} MemoryStats;

// Linked list node for storing user session information
typedef struct UserNode {
    char username[256];  // Username
    char utmp_line[32];  // Terminal line (e.g., tty/pts)
    char hostname[256];  // Hostname for the user session
    struct UserNode *next;  // Pointer to next node in the list
} UserNode;




// Parses command line arguments and sets corresponding flags
void parse_arguments(int argc, char *argv[], int *samples, int *tdelay, int *system_flag, int *user_flag, int *graphics_flag, int *sequential_flag);

// Displays the header information for each sample interval
void display_header(int sample_number, int samples, int tdelay, int sequential_flag, int system_flag);

// Gathers and stores memory statistics into the provided array at the specified index
void gather_memory_stats(MemoryStats *memory_stats_array, int index);

// Displays memory statistics based on the array, considering sequential and graphics flags
void display_memory_stats(MemoryStats *memory_stats_array, int samples, int currentSample, int sequential, int graphics_flag, double *prev_virt);

// Retrieves and prints the number of CPU cores
void get_cpu_cores(void);

// Retrieves idle and total CPU times for calculating CPU usage
void get_cpu_idle_total_times(unsigned long *idle_time, unsigned long *total_time);

// Calculates and prints CPU usage between two time intervals
double calculate_and_print_cpu_usage(unsigned long idle_start, unsigned long idle_end, unsigned long total_start, unsigned long total_end);

// Updates graphical representation of CPU usage
void update_cpu_graphics(double cpu_usage, int sample_index, char cpu_graphics_arr[][1024], int samples);

// Prints CPU usage graphics for all samples up to the current one
void print_cpu_graphics(int currentSample, int sequential, char cpu_graphics_arr[][1024], int samples);

// Prints system information such as OS version, machine name, and uptime
void print_system_info(void);

// Launches a process to gather and report memory stats, communicating via pipes
void launchMemoryStatsProcess(int pipefd[2], MemoryStats *memory_stats_array, int samples, int currentSample, int sequential_flag, int graphics_flag, double *prev_virt);

// Prints the list of user sessions
void print_user_list(UserNode *head);

// Frees the memory allocated for the user list
void free_user_list(UserNode *head);

// Appends a new user session to the list
UserNode* append_user(UserNode* head, const char* username, const char* utmp_line, const char* hostname);

// Launches a process to gather and report user session stats, communicating via pipes
UserNode* launchUserStatsProcess(int pipefd[2]);

// Launches a process to gather and report CPU stats, communicating via pipes, and optionally prints them
void launchCpuStatsProcess(int pipefd[2], unsigned long idle_start, unsigned long total_start, int graphics_flag, int sample_index, char cpu_graphics_arr[][1024], int samples, int sequential_flag);

// End of the include guard
#endif

