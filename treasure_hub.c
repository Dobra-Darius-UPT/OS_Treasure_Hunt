#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

#define MAX_INPUT 256
#define COMMAND_FILE ".monitor_command"
#define RESPONSE_FILE ".monitor_response"

volatile pid_t monitor_pid = 0;
volatile bool monitor_running = false;
volatile bool waiting_for_monitor = false;

// Function prototypes
void handle_sigchld(int sig);
void start_monitor();
void stop_monitor();
void list_hunts();
void list_treasures();
void view_treasure();
void send_command_to_monitor(const char *cmd, const char *arg);
void read_monitor_response();
void setup_signal_handlers();

// Signal handler for SIGCHLD
void handle_sigchld(int sig) {
    (void)sig; // Unused parameter
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    
    if (pid == monitor_pid) {
        monitor_running = false;
        monitor_pid = 0;
        waiting_for_monitor = false;
        
        if (WIFEXITED(status)) {
            printf("\nMonitor process terminated with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("\nMonitor process was terminated by signal %d\n", WTERMSIG(status));
        }
    }
}

// Start the monitor process
void start_monitor() {
    if (monitor_running) {
        printf("Monitor is already running\n");
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    
    if (pid == 0) { // Child process (monitor)
        execl("./treasure_monitor", "treasure_monitor", NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else { // Parent process
        monitor_pid = pid;
        monitor_running = true;
        printf("Monitor started with PID %d\n", pid);
    }
}

// Stop the monitor process
void stop_monitor() {
    if (!monitor_running) {
        printf("No monitor is running\n");
        return;
    }
    
    if (waiting_for_monitor) {
        printf("Already waiting for monitor to stop\n");
        return;
    }
    
    waiting_for_monitor = true;
    kill(monitor_pid, SIGUSR1);
    printf("Sent stop signal to monitor\n");
}

// Send list_hunts command to monitor
void list_hunts() {
    if (!monitor_running) {
        printf("No monitor is running\n");
        return;
    }
    send_command_to_monitor("list_hunts", NULL);
}

// Send list_treasures command to monitor
void list_treasures() {
    if (!monitor_running) {
        printf("No monitor is running\n");
        return;
    }
    
    printf("Enter hunt ID: ");
    char hunt_id[MAX_INPUT];
    fgets(hunt_id, MAX_INPUT, stdin);
    hunt_id[strcspn(hunt_id, "\n")] = '\0'; // Remove newline
    
    send_command_to_monitor("list_treasures", hunt_id);
}

// Send view_treasure command to monitor
void view_treasure() {
    if (!monitor_running) {
        printf("No monitor is running\n");
        return;
    }
    
    printf("Enter hunt ID: ");
    char hunt_id[MAX_INPUT];
    fgets(hunt_id, MAX_INPUT, stdin);
    hunt_id[strcspn(hunt_id, "\n")] = '\0'; // Remove newline
    
    printf("Enter treasure ID: ");
    char treasure_id[MAX_INPUT];
    fgets(treasure_id, MAX_INPUT, stdin);
    treasure_id[strcspn(treasure_id, "\n")] = '\0'; // Remove newline
    
    char arg[MAX_INPUT * 2];
    snprintf(arg, sizeof(arg), "%s %s", hunt_id, treasure_id);
    
    send_command_to_monitor("view_treasure", arg);
}

// Send command to monitor via file and signal
void send_command_to_monitor(const char *cmd, const char *arg) {
    // Write command to file
    FILE* cmd_file = fopen(COMMAND_FILE, "w");
    if (!cmd_file) {
        perror("fopen command file");
        return;
    }
    
    if (arg) {
        fprintf(cmd_file, "%s %s", cmd, arg);
    } else {
        fprintf(cmd_file, "%s", cmd);
    }
    fclose(cmd_file);
    
    // Send signal to monitor
    kill(monitor_pid, SIGUSR2);
    
    // Wait for response
    read_monitor_response();
}

// Read and display monitor response
void read_monitor_response() {
    usleep(100000); // Small delay to allow monitor to write response
    
    FILE *response = fopen(RESPONSE_FILE, "r");
    if (!response) {
        perror("fopen response file");
        return;
    }
    
    printf("\n=== Monitor Response ===\n");
    char line[MAX_INPUT];
    while (fgets(line, MAX_INPUT, response)) {
        printf("%s", line);
    }
    fclose(response);
    printf("=======================\n");
}

// Setup signal handlers
void setup_signal_handlers() {
    struct sigaction sa;
    
    // SIGCHLD handler
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
    
    // Ignore SIGUSR1 and SIGUSR2 in hub (handled in monitor)
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGUSR1, &sa, NULL) == -1 || sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1/SIGUSR2");
        exit(EXIT_FAILURE);
    }
}

// Main interactive loop
int main() {
    setup_signal_handlers();
    
    printf("=== Treasure Hunt Hub ===\n");
    printf("Commands(in a possible usage order):\n 1.start_monitor\n 2.list_hunts\n 3.list_treasures\n 4.view_treasure\n 5.stop_monitor\n 6.exit\n");
    
    while (1) {
        printf("\nhub> ");
        fflush(stdout);
        
        char input[MAX_INPUT];
        if (!fgets(input, MAX_INPUT, stdin)) {
            break; // EOF or error
        }
        
        input[strcspn(input, "\n")] = '\0'; // Remove newline
        
        if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(input, "list_hunts") == 0) {
            list_hunts();
        } else if (strcmp(input, "list_treasures") == 0) {
            list_treasures();
        } else if (strcmp(input, "view_treasure") == 0) {
            view_treasure();
        } else if (strcmp(input, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Error: Monitor is still running\n");
            } else {
                break;
            }
        } else {
            printf("Unknown command\n");
        }
    }
    
    // Cleanup
    unlink(COMMAND_FILE);
    unlink(RESPONSE_FILE);
    
    return 0;
}