#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include "stats_functions.h"


// Defining the long_options array here
static struct option long_options[] = {
    {"system",      no_argument,       0, 's'},
    {"user",        no_argument,       0, 'u'},
    {"graphics",    no_argument,       0, 'g'},
    {"sequential",  no_argument,       0, 'q'},
    {"samples",     required_argument, 0, 'n'},
    {"tdelay",      required_argument, 0, 't'},
    {0, 0, 0, 0}  // Sentinel to mark the end of the array
};

/*
 * Function: parse_arguments
 * ----------------------------
 * Parses the command-line arguments passed to the program and sets the respective flags and values.
 *
 * argc: Number of arguments.
 * argv: Array of argument strings.
 * samples: Pointer to store the number of samples to take.
 * tdelay: Pointer to store the delay between samples.
 * system_flag: Pointer to flag indicating system stats collection.
 * user_flag: Pointer to flag indicating user stats collection.
 * graphics_flag: Pointer to flag indicating whether to display graphics.
 * sequential_flag: Pointer to flag indicating whether to run in sequential mode.
 */
void parse_arguments(int argc, char *argv[], int *samples, int *tdelay, int *system_flag, int *user_flag, int *graphics_flag, int *sequential_flag) {
    // Initialization of variables for getopt_long
    int option_index = 0;
    int c;
    int samples_flag = 0;
    int tdelay_flag = 0;

    // Loop through each argument and set flags or values based on the options
    while ((c = getopt_long(argc, argv, "sugqn::t::", long_options, &option_index)) != -1) {
        switch (c) {
            // Set flags based on the command line options
            case 's': *system_flag = 1; break;
            case 'u': *user_flag = 1; break;
            case 'g': *graphics_flag = 1; break;
            case 'q': *sequential_flag = 1; break;
            // Set samples and tdelay based on provided values or defaults
            case 'n': 
                if (optarg) {
                    *samples = atoi(optarg);
                    samples_flag = 1;
                }
                break;
            case 't': 
                if (optarg) {
                    *tdelay = atoi(optarg);
                    tdelay_flag = 1;
                }
                break;
        }
    }

    // Handle positional arguments for samples and tdelay
    // These are used when the user provides arguments without flags
    for (int pa = optind, index = 0; pa < argc && index < 2; pa++, index++) {
        switch (index) {
            case 0: // First positional argument corresponds to 'samples'
                if (!samples_flag) {
                    *samples = atoi(argv[pa]);
                }
                break;
            case 1: // Second positional argument corresponds to 'tdelay'
                if (!tdelay_flag) {
                    *tdelay = atoi(argv[pa]);
                }
                break;
        }
    }
}

/*
 * Function: display_header
 * ----------------------------
 * Displays the header information for each sample, including the iteration or total samples and delay.
 *
 * sample_number: The current sample number being processed.
 * samples: Total number of samples to take.
 * tdelay: Delay between samples.
 * sequential_flag: Flag indicating whether to run in sequential mode.
 * system_flag: Flag indicating system stats collection.
 */
void display_header(int sample_number, int samples, int tdelay, int sequential_flag, int system_flag) {
    struct rusage r_usage;
    // Get resource usage to display memory usage of the tool itself
    getrusage(RUSAGE_SELF, &r_usage);
    
    if (sequential_flag) {
        printf(">>> iteration %d\n", sample_number);
    } else {
        printf("\033[H\033[2J"); // ANSI escape code to clear the screen
        printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay);
    }
    printf(" Memory usage: %ld kilobytes\n", r_usage.ru_maxrss);
}

// memory stuff

/*
 * Function: gather_memory_stats
 * ----------------------------
 * Gathers memory statistics and stores them in the specified index of the memory stats array.
 *
 * memory_stats_array: Array to store memory statistics.
 * index: Index in the array to store the gathered stats.
 */
void gather_memory_stats(MemoryStats *memory_stats_array, int index) {
    struct sysinfo info;
    // Get system information
    if (sysinfo(&info) != 0) {
        perror("sysinfo: error reading system statistics");
        exit(EXIT_FAILURE);
    }

    // Convert memory usage to gigabytes and store in the array
    double bytes_to_gb = 1.0 / (1024 * 1024 * 1024);
    memory_stats_array[index].phys_used = (info.totalram - info.freeram) * bytes_to_gb;
    memory_stats_array[index].phys_total = info.totalram * bytes_to_gb;
    memory_stats_array[index].virt_used = (info.totalram - info.freeram + info.totalswap - info.freeswap) * bytes_to_gb;
    memory_stats_array[index].virt_total = (info.totalram + info.totalswap) * bytes_to_gb;
}

/*
 * Function: append_graphical_representation
 * ----------------------------
 * Appends a graphical representation based on the difference between the current and previous virtual memory usage.
 *
 * diff: The difference in virtual memory usage.
 * currentVirtUsed: The current virtual memory usage.
 * prev_virt: Pointer to the previous virtual memory usage, to be updated.
 */
void append_graphical_representation(double diff, double currentVirtUsed, double *prev_virt) {
    int bars = fabs(diff) * 100; // Calculate the number of bars to represent the change
    printf("   |");
    if (diff >= 0.01) {
        for (int j = 0; j < bars && j < 100; j++) printf("#");
        printf("*");
    } else if (diff <= -0.01) {
        for (int j = 0; j < bars && j < 100; j++) printf("@");
        printf("*");
    } else {
        printf("o");
    }
    printf(" %.2f (%.2f)", diff, currentVirtUsed);
    *prev_virt = currentVirtUsed; // Update the previous virtual memory usage
}

/*
 * Function: display_memory_stats
 * ----------------------------
 * Displays the memory statistics for either the current sample (sequential mode) or all samples up to the current one.
 *
 * memory_stats_array: Array containing memory statistics.
 * samples: Total number of samples.
 * currentSample: The current sample index being processed.
 * sequential: Flag indicating whether to run in sequential mode.
 * graphics_flag: Flag indicating whether to display graphical representation.
 * prev_virt: Pointer to the previous virtual memory usage.
 */
void display_memory_stats(MemoryStats *memory_stats_array, int samples, int currentSample, int sequential, int graphics_flag, double *prev_virt) {
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");

    if (sequential) {
        // In sequential mode, display stats for the current sample only
        for (int i = 0; i < samples; ++i) {
            if (i == currentSample) {
                double diff = (i == 0) ? 0 : memory_stats_array[i].virt_used - *prev_virt;
                printf("%.2f GB / %.2f GB -- %.2f GB / %.2f GB", memory_stats_array[i].phys_used, memory_stats_array[i].phys_total, memory_stats_array[i].virt_used, memory_stats_array[i].virt_total);
                if (graphics_flag) {
                    append_graphical_representation(diff, memory_stats_array[i].virt_used, prev_virt);
                }
                printf("\n");
            } else {
                printf("\n");
            }
        }
    } else {
        // In non-sequential mode, display stats for all samples up to the current one
        for (int i = 0; i <= currentSample; ++i) {
            double diff = (i == 0) ? 0 : memory_stats_array[i].virt_used - memory_stats_array[i - 1].virt_used;
            printf("%.2f GB / %.2f GB -- %.2f GB / %.2f GB", memory_stats_array[i].phys_used, memory_stats_array[i].phys_total, memory_stats_array[i].virt_used, memory_stats_array[i].virt_total);
            if (graphics_flag) {
                append_graphical_representation(diff, memory_stats_array[i].virt_used, prev_virt);
            }
            printf("\n");
        }
        // Fill with new lines for the remaining samples
        for (int i = currentSample + 1; i < samples; ++i) {
            printf("\n");
        }
    }
    // Update prev_virt for the next iteration
    if (graphics_flag && currentSample < samples - 1) {
        *prev_virt = memory_stats_array[currentSample].virt_used;
    }
}


// CPU stuff
/**
 * @brief Retrieves and prints the number of online processor cores in the system.
 */
void get_cpu_cores() {
    long n_processors = sysconf(_SC_NPROCESSORS_ONLN); // Get the number of online processors
    printf("Number of cores: %ld\n", n_processors);
}

/**
 * Reads the system's CPU time statistics from /proc/stat and extracts the total and idle CPU times.
 * These times are essential for calculating CPU usage over a period.
 *
 * @param idle_time Pointer to store the calculated idle CPU time.
 * @param total_time Pointer to store the calculated total CPU time.
 */
void get_cpu_idle_total_times(unsigned long *idle_time, unsigned long *total_time) {
    FILE *fp; // File pointer to open /proc/stat
    unsigned long times[7]; // Array to store the different times read from /proc/stat
    char buffer[1024]; // Buffer to store the line read from /proc/stat

    // Attempt to open /proc/stat for reading CPU times
    fp = fopen("/proc/stat", "r");
    if (!fp) { // If opening the file fails, print an error message and exit
        perror("Failed to open /proc/stat");
        exit(EXIT_FAILURE);
    }

    // Attempt to read the first line from /proc/stat into the buffer
    if (!fgets(buffer, sizeof(buffer), fp)) {
        perror("Failed to read from /proc/stat"); // Print error if reading fails
        fclose(fp); // Close the file pointer to clean up
        exit(EXIT_FAILURE);
    }

    fclose(fp); // Close the file after reading the necessary line

    // Parse the read line for CPU times using sscanf
    // Expects to read 7 unsigned long values into the times array
    if (sscanf(buffer, "cpu  %lu %lu %lu %lu %lu %lu %lu",
               &times[0], &times[1], &times[2], &times[3], 
               &times[4], &times[5], &times[6]) != 7) {
        fprintf(stderr, "Error: Expected to read 7 CPU time values\n"); // Error if not all values are read
        exit(EXIT_FAILURE);
    }

    // Assign the read values to idle and total times
    // The fourth value (times[3]) is the idle time
    *idle_time = times[3];
    // Total time is the sum of all read values, representing various CPU states
    *total_time = times[0] + times[1] + times[2] + times[3] + times[4] + times[5] + times[6];
}


/**
 * Calculates and prints the CPU usage percentage based on start and end idle/total times.
 *
 * @param idle_start Starting idle CPU time.
 * @param idle_end Ending idle CPU time.
 * @param total_start Starting total CPU time.
 * @param total_end Ending total CPU time.
 * @return The calculated CPU usage percentage.
 */
double calculate_and_print_cpu_usage(unsigned long idle_start, unsigned long idle_end, 
                                     unsigned long total_start, unsigned long total_end) {
    // Calculate the differences in total and idle times
    unsigned long total_diff = total_end - total_start;
    unsigned long idle_diff = idle_end - idle_start;
    double cpu_usage;

    // Prevent division by zero and calculate usage
    if (total_diff == 0) {
        cpu_usage = 0.0; // Handle the case where there is no difference, preventing division by zero
    } else {
        // Calculate usage as a percentage of the non-idle time over total time
        cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
    }

    // Print the calculated CPU usage
    printf(" total CPU use = %.2f%%\n", cpu_usage);
    return cpu_usage; // Return the CPU usage value for potential further use
}

/**
 * Updates a graphical representation of CPU usage for a specific sample.
 *
 * @param cpu_usage The CPU usage percentage.
 * @param sample_index Index of the current sample in the graphical array.
 * @param cpu_graphics_arr Array storing the graphical representation strings.
 * @param samples Total number of samples (unused in this function but included for consistency/future use).
 */
void update_cpu_graphics(double cpu_usage, int sample_index, char cpu_graphics_arr[][1024], int samples) {
    // Calculate the number of bars to represent the CPU usage graphically
    int num_bars = cpu_usage + 3; // Example logic to determine the length of the graphical representation

    // Clear the existing content for the current sample
    memset(cpu_graphics_arr[sample_index], '\0', sizeof(cpu_graphics_arr[sample_index]));
    // Start with some padding for alignment
    snprintf(cpu_graphics_arr[sample_index], sizeof(cpu_graphics_arr[sample_index]), "         ");
    // Append bars to the string based on calculated CPU usage
    for (int i = 0; i < num_bars; i++) {
        strcat(cpu_graphics_arr[sample_index], "|");
    }
    // Append the CPU usage percentage at the end of the graphical representation
    char usage_str[32];
    snprintf(usage_str, sizeof(usage_str), " %.2f ", cpu_usage);
    strcat(cpu_graphics_arr[sample_index], usage_str);
}

/**
 * Prints the graphical representation of CPU usage for each sample up to the current one.
 *
 * @param currentSample The index of the current sample being processed.
 * @param sequential Whether the output should be in sequential mode or not.
 * @param cpu_graphics_arr The array holding graphical representations of CPU usage.
 * @param samples The total number of samples.
 */
void print_cpu_graphics(int currentSample, int sequential, char cpu_graphics_arr[][1024], int samples) {
    // Handle sequential and non-sequential modes of operation
    if (sequential) {
        // In sequential mode, print the graphic for the current sample only
        for (int i = 0; i <= currentSample; i++) {
            if (i == currentSample) {
                printf("%s\n", cpu_graphics_arr[i]);
            } else {
                printf("\n"); // Print empty lines for previous samples
            }
        }
    } else {
        // In non-sequential mode, print the graphics for all samples up to the current one
        for (int i = 0; i <= currentSample; i++) {
            printf("%s\n", cpu_graphics_arr[i]);
        }
    }
}

// System stuff

/**
 * Prints detailed system information including system name, machine name,
 * version, release, architecture, and uptime.
 */
void print_system_info() {
    struct utsname system_info; // To store system information
    struct sysinfo sys_info; // To store system uptime and loads

    // Retrieve system information
    if (uname(&system_info) < 0) {
        perror("uname");
        exit(EXIT_FAILURE);
    }

    // Retrieve system uptime
    if (sysinfo(&sys_info) < 0) {
        perror("sysinfo");
        exit(EXIT_FAILURE);
    }

    // Calculate uptime components
    int days = sys_info.uptime / (24 * 3600);
    int hours = (sys_info.uptime % (24 * 3600)) / 3600;
    int minutes = (sys_info.uptime % 3600) / 60;
    int seconds = sys_info.uptime % 60;

    // Print system information
    printf("### System Information ###\n");
    printf(" System Name = %s\n", system_info.sysname);
    printf(" Machine Name = %s\n", system_info.nodename);
    printf(" Version = %s\n", system_info.version);
    printf(" Release = %s\n", system_info.release);
    printf(" Architecture = %s\n", system_info.machine);
    printf(" System running since last reboot: %d days %02d:%02d:%02d (%d:%02d:%02d)\n",
           days, hours, minutes, seconds, hours + (days * 24), minutes, seconds);
}

// launch memory 

/**
 * Launches a child process to gather and report memory statistics back to the parent process.
 * The child process gathers memory statistics and writes them to a pipe.
 * The parent process reads these statistics from the pipe and displays them.
 *
 * @param pipefd Pipe file descriptor array used for interprocess communication.
 * @param memory_stats_array Array to store memory statistics.
 * @param samples Total number of samples to be taken.
 * @param currentSample Index of the current sample being processed.
 * @param sequential_flag Flag indicating if output should be sequential.
 * @param graphics_flag Flag indicating if graphical representation is enabled.
 * @param prev_virt Pointer to the previous virtual memory usage, used for graphical representation.
 */
void launchMemoryStatsProcess(int pipefd[2], MemoryStats *memory_stats_array, int samples, 
                              int currentSample, int sequential_flag, int graphics_flag, 
                              double *prev_virt) {
    pid_t cpid; // Child process ID
    char buffer[1024]; // Buffer to hold formatted memory stats data

    // Forking the process
    cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (cpid == 0) { // Child process: gathers and sends memory stats
        close(pipefd[0]); // Close unused read end of the pipe
        gather_memory_stats(memory_stats_array, currentSample); // Gather stats
        // Format stats into a string
        snprintf(buffer, sizeof(buffer), "%.2f %.2f %.2f %.2f",
                 memory_stats_array[currentSample].phys_used,
                 memory_stats_array[currentSample].phys_total,
                 memory_stats_array[currentSample].virt_used,
                 memory_stats_array[currentSample].virt_total);
        write(pipefd[1], buffer, strlen(buffer) + 1); // Send to parent
        close(pipefd[1]); // Signal end of data
        exit(EXIT_SUCCESS);
    } else { // Parent process: reads and displays memory stats
        close(pipefd[1]); // Close unused write end
        read(pipefd[0], buffer, sizeof(buffer)); // Read data from child
        // Optionally parse received data into the memory_stats_array
        sscanf(buffer, "%lf %lf %lf %lf",
               &memory_stats_array[currentSample].phys_used,
               &memory_stats_array[currentSample].phys_total,
               &memory_stats_array[currentSample].virt_used,
               &memory_stats_array[currentSample].virt_total);
        display_memory_stats(memory_stats_array, samples, currentSample, 
                             sequential_flag, graphics_flag, prev_virt); // Display stats
        close(pipefd[0]); // Close read end
        wait(NULL); // Wait for child to finish
    }
}

// launch user

/**
 * Prints the list of users.
 * Iterates through the linked list of UserNode, printing each user's details.
 * 
 * @param head Pointer to the head of the linked list of users.
 */
void print_user_list(UserNode *head) {
    UserNode *current = head; // Pointer to traverse the linked list
    printf("### Sessions/users ###\n");
    // Loop through the linked list and print user details
    while (current != NULL) {
        printf("%s\t%s\t(%s)\n", current->username, current->utmp_line, current->hostname);
        current = current->next; // Move to the next node
    }
}

/**
 * Frees the memory allocated for the user list.
 * Iterates through the linked list of UserNode, freeing each node.
 * 
 * @param head Pointer to the head of the linked list of users.
 */
void free_user_list(UserNode *head) {
    UserNode *current = head; // Pointer to traverse the linked list
    UserNode *next; // Pointer to hold the next node

    // Loop through the linked list and free each node
    while (current != NULL) {
        next = current->next; // Save the next node
        free(current); // Free the current node
        current = next; // Move to the next node
    }
}

/**
 * Appends a new user to the end of the user list.
 * Dynamically allocates memory for a new UserNode, sets its details,
 * and appends it to the end of the linked list. If the list is empty, 
 * the new node becomes the head of the list.
 * 
 * @param head The head of the user list to append to. If NULL, the new node becomes the head.
 * @param username The username to append.
 * @param utmp_line The terminal line the user is connected to.
 * @param hostname The hostname from which the user is connected.
 * @return Pointer to the head of the linked list.
 */
UserNode* append_user(UserNode* head, const char* username, const char* utmp_line, const char* hostname) {
    UserNode* newNode = malloc(sizeof(UserNode)); // Allocate memory for the new node
    if (newNode == NULL) { // Check for successful memory allocation
        perror("Failed to allocate memory for new user node");
        exit(EXIT_FAILURE);
    }

    // Set user details
    strncpy(newNode->username, username, sizeof(newNode->username));
    strncpy(newNode->utmp_line, utmp_line, sizeof(newNode->utmp_line));
    strncpy(newNode->hostname, hostname, sizeof(newNode->hostname));
    newNode->next = NULL; // New node is the last node, so next is NULL

    if (head == NULL) { // If the list is empty, new node is now the head
        return newNode;
    }

    // Find the last node and append the new node
    UserNode* current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = newNode;

    return head; // Return the head of the list
}

/**
 * Launches a process to gather and report user statistics.
 * Utilizes a pipe to communicate user statistics from a forked child process
 * back to the parent process. It reads from utmp to list active users,
 * then appends each user to a linked list which is returned.
 * 
 * @param pipefd An array of two integers used as file descriptors for the pipe.
 * @return Head pointer to the linked list of users (UserNode*).
 */
UserNode* launchUserStatsProcess(int pipefd[2]) {
    pid_t cpid; // PID of the child process

    // Create a pipe and handle failure
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork the process
    cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) { // In child process
        close(pipefd[0]); // Close read end; not used by child

        // Begin reading user entries
        struct utmp* u;
        setutent(); // Rewind to the start of utmp file
        FILE* stream = fdopen(pipefd[1], "w"); // Open write-end of pipe as a file

        // Read each user entry and write to pipe
        while ((u = getutent()) != NULL) {
            if (u->ut_type == USER_PROCESS) {
                fprintf(stream, "%s %s %s\n", u->ut_user, u->ut_line, u->ut_host);
            }
        }
        endutent(); // Close utmp file
        fclose(stream); // Close stream and pipe write-end, then exit
        exit(EXIT_SUCCESS);
    } else { // In parent process
        close(pipefd[1]); // Close write end; not used by parent
        FILE* stream = fdopen(pipefd[0], "r"); // Open read-end of pipe as a file

        UserNode* head = NULL; // Head of the users linked list
        char username[256], utmp_line[32], hostname[256];

        // Read user data from pipe and append to linked list
        while (fscanf(stream, "%255s %31s %255s\n", username, utmp_line, hostname) == 3) {
            head = append_user(head, username, utmp_line, hostname);
        }

        fclose(stream); // Close stream and read-end of pipe
        wait(NULL); // Wait for child to finish

        return head; // Return head of the linked list
    }
}

// launch cpu


/**
 * Launches a process to calculate and report CPU usage statistics.
 * Forks a child process to gather CPU usage data, which is then sent back
 * to the parent process via a pipe. The CPU usage is calculated based on 
 * idle and total times at the start and end of an interval.
 * 
 * @param pipefd Array of two integers for the pipe's file descriptors.
 * @param idle_start Idle CPU time at the start of the interval.
 * @param total_start Total CPU time at the start of the interval.
 * @param graphics_flag Flag indicating if graphics mode is enabled.
 * @param sample_index Index of the current sample in the monitoring cycle.
 * @param cpu_graphics_arr Array storing graphical representation of CPU usage.
 * @param samples Total number of samples to be taken.
 * @param sequential_flag Flag indicating if output is sequential.
 */
void launchCpuStatsProcess(int pipefd[2], unsigned long idle_start, unsigned long total_start, int graphics_flag, int sample_index, char cpu_graphics_arr[][1024], int samples, int sequential_flag) {
    pid_t cpid; // PID of the child process
    char buffer[256]; // Buffer to store CPU usage data

    // Fork the process
    cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) { // In child process
        close(pipefd[0]); // Close read end; not used by child

        // Gather CPU stats at the end of the interval
        unsigned long idle_end, total_end;
        get_cpu_idle_total_times(&idle_end, &total_end);

        // Calculate CPU usage based on idle and total time differences
        double cpu_usage = calculate_and_print_cpu_usage(idle_start, idle_end, total_start, total_end);

        // Send CPU usage to the parent process
        snprintf(buffer, sizeof(buffer), "%.2f", cpu_usage);
        write(pipefd[1], buffer, strlen(buffer));

        close(pipefd[1]); // Close write end of the pipe and exit
        exit(EXIT_SUCCESS);
    } else { // In parent process
        close(pipefd[1]); // Close write end; not used by parent

        // Wait for the child process to complete
        wait(NULL);

        // Read CPU usage data sent by the child process
        read(pipefd[0], buffer, sizeof(buffer));
        double cpu_usage;
        sscanf(buffer, "%lf", &cpu_usage);

        // Update and print CPU graphics if enabled
        if (graphics_flag) {
            update_cpu_graphics(cpu_usage, sample_index, cpu_graphics_arr, samples);
            print_cpu_graphics(sample_index, sequential_flag, cpu_graphics_arr, samples);
        }

        close(pipefd[0]); // Close read end of the pipe
    }
}





