#include "stats_functions.h"

/**
 * Handles the SIGINT signal by prompting the user to confirm if they want to exit the program.
 * @param sig_num The signal number (expected to be SIGINT).
 */
void sigint_handler(int sig_num) {
    // Temporarily ignore SIGINT signals
    signal(SIGINT, SIG_IGN);

    printf("\nDo you want to quit? [y/N]: ");
    char response[10];
    fgets(response, sizeof(response), stdin);

    if (response[0] == 'y' || response[0] == 'Y') {
        printf("Exiting program...\n");
        exit(0); // Exit the program if the user confirms
    } else {
        printf("Continuing execution...\n");
        // Re-enable the SIGINT handler for future signals
        signal(SIGINT, sigint_handler);
    }
}

/**
 * The main entry point of the program. Initializes the application, sets up signal handling,
 * and manages the execution flow based on user input and signal events.
 */
int main(int argc, char *argv[]) {
    // Ignore Ctrl-Z signals (SIGTSTP)
    signal(SIGTSTP, SIG_IGN);

    // Handle Ctrl-C (SIGINT) using the sigint_handler function
    signal(SIGINT, sigint_handler);

    // Initialize variables based on user input or default values
    int samples = 10, tdelay = 1;
    int system_flag = 0, user_flag = 0, graphics_flag = 0, sequential_flag = 0;
    double prev_virt = 0.00; // Used for graphical memory usage display

    // Parse command-line arguments to configure the program's execution
    parse_arguments(argc, argv, &samples, &tdelay, &system_flag, &user_flag, &graphics_flag, &sequential_flag);

    // Allocate memory for storing statistics and graphical representations
    MemoryStats memory_stats_array[samples];
    char cpu_graphics_arr[samples][1024];

    // Pipe file descriptors for inter-process communication
    int mem_pipefd[2], cpu_pipefd[2], user_pipefd[2];

    // Main loop to collect and display system statistics for the number of specified samples
    for (int i = 0; i < samples; ++i) {
        // Create pipes for memory, CPU, and user statistics
        if (pipe(mem_pipefd) == -1 || pipe(cpu_pipefd) == -1 || pipe(user_pipefd) == -1) {
            fprintf(stderr, "Pipe creation failed\n");
            return 1; // Exit if pipe creation fails
        }

        // Collect initial CPU usage data
        unsigned long idle_start, total_start;
        get_cpu_idle_total_times(&idle_start, &total_start);
        sleep(tdelay); // Wait for the specified delay time

        // Display header information for the current sample
        display_header(i, samples, tdelay, sequential_flag, system_flag);

        // Process and display memory, user, and CPU statistics
        if (!user_flag || (user_flag && system_flag)) {
            printf("---------------------------------------\n");
            launchMemoryStatsProcess(mem_pipefd, memory_stats_array, samples, i, sequential_flag, graphics_flag, &prev_virt);
            if ((user_flag && system_flag) || !system_flag) {
                printf("---------------------------------------\n");
                UserNode* userHead = launchUserStatsProcess(user_pipefd);
                print_user_list(userHead);
                free_user_list(userHead); // Clean up the user list
                printf("---------------------------------------\n");
            }
            get_cpu_cores();
            launchCpuStatsProcess(cpu_pipefd, idle_start, total_start, graphics_flag, i, cpu_graphics_arr, samples, sequential_flag);
        } else {
            printf("---------------------------------------\n");
            UserNode* userHead = launchUserStatsProcess(user_pipefd);
            print_user_list(userHead);
            free_user_list(userHead); // Clean up the user list
            printf("---------------------------------------\n");
        }
    }

    // Display final system information after processing all samples
    printf("---------------------------------------\n");
    print_system_info();
    printf("---------------------------------------\n");
    return 0; // End of program
}



