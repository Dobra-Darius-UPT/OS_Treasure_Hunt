// ========================== treasure_monitor.c (stdout flush version) ==========================
#include "treasure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>


#define MAX_INPUT_SIZE 512
#define COMMAND_FILE ".monitor_command"

volatile bool running = true;

// Function prototypes
void handle_sigusr1(int sig);
void handle_sigusr2(int sig);
void setup_signal_handlers();
void process_command();
void list_all_hunts();
void list_hunt_treasures(const char *hunt_id);
void calculate_score();
void view_specific_treasure(const char *hunt_id, const char *treasure_id);

// Signal handler for SIGUSR1 (stop)
void handle_sigusr1(int sig) {
    (void)sig;
    running = false;
}

// Signal handler for SIGUSR2 (new command)
void handle_sigusr2(int sig) {
    (void)sig;
    //usleep(5000);
    process_command();
}

// Setup signal handlers
void setup_signal_handlers() {
    struct sigaction sa;

    // SIGUSR1 handler
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    // SIGUSR2 handler
    sa.sa_handler = handle_sigusr2;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }

    // Ignore SIGCHLD
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
}

// Process command from hub
void process_command() {
    char cmd[MAX_INPUT_SIZE] = {0};

    FILE *cmd_file = fopen(COMMAND_FILE, "r");
    if (!cmd_file) {
        perror("fopen command file");
        return;
    }

    if (!fgets(cmd, MAX_INPUT_SIZE, cmd_file)) {
        fclose(cmd_file);
        unlink(COMMAND_FILE);
        return;
    }
    fclose(cmd_file);
    unlink(COMMAND_FILE);

    cmd[strcspn(cmd, "\n")] = '\0';

    char *space = strchr(cmd, ' ');
    char *arg = NULL;
    if (space) {
        *space = '\0';
        arg = space + 1;
    }

    if (strcmp(cmd, "list_hunts") == 0) {
        list_all_hunts();
    } else if (strcmp(cmd, "list_treasures") == 0 && arg) {
        list_hunt_treasures(arg);
    } else if (strcmp(cmd, "view_treasure") == 0 && arg) {
        char *treasure_space = strchr(arg, ' ');
        if (treasure_space) {
            *treasure_space = '\0';
            char *treasure_id = treasure_space + 1;
            view_specific_treasure(arg, treasure_id);
        } else {
            printf("Error: Missing treasure ID\n");
        }
    } else if (strcmp(cmd, "calculate_score") == 0) {
        calculate_score(); 
    }else {
        printf("Error: Unknown or empty command: '%s'\n", cmd);
    }
    fflush(stdout);
}

// List all hunts
void list_all_hunts() {
    DIR *dir = opendir(".");
    struct dirent *entry;
    printf("Available hunts:\n");
    if (!dir) {
        perror("opendir");
        printf("Error: Could not list hunts\n");
        return;
    }

    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[MAX_INPUT_SIZE];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
            struct stat st;
            if (stat(path, &st) == 0) {
                int num_treasures = st.st_size / sizeof(Treasure);
                printf("- %s (%d treasures)\n", entry->d_name, num_treasures);
                count++;
            }
        }
    }
    closedir(dir);
    if (count == 0) {
        printf("No hunts found\n");
    }
    fflush(stdout);
}

// List all treasures in a hunt
void list_hunt_treasures(const char *hunt_id) {
    char path[MAX_INPUT_SIZE];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    FILE *treasure_file = fopen(path, "rb");
    if (!treasure_file) {
        printf("Error: Could not open hunt '%s'\n", hunt_id);
        return;
    }
    printf("Treasures in hunt '%s':\n", hunt_id);
    Treasure t;
    while (fread(&t, sizeof(Treasure), 1, treasure_file) == 1) {
        printf("- ID: %s, User: %s, Value: %d\n", t.id, t.user_name, t.value);
    }
    fclose(treasure_file);
    fflush(stdout);
}

// Calculate the score per player
void calculate_score() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    pid_t child_pids[100];
    int child_count = 0;

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {

            int fd[2];
            pipe(fd);
            pid_t pid = fork();

            if (pid == 0) {
                // Child
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                execl("./score_calc", "score_calc", entry->d_name, NULL);
                perror("execl");
                exit(EXIT_FAILURE);
            } else {
                // Parent
                close(fd[1]);
                char buffer[512];
                ssize_t n;
                while ((n = read(fd[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[n] = '\0';
                    printf("%s", buffer);
                }
                close(fd[0]);

                child_pids[child_count++] = pid;

                printf("\n");
                fflush(stdout);
            }
        }
    }
    closedir(dir);

    for (int i = 0; i < child_count; ++i) {
        int status;
        waitpid(child_pids[i], &status, 0);
        printf("Waited child %d\n", child_pids[i]);
    }

    fflush(stdout);
}

// View specific treasure details
void view_specific_treasure(const char *hunt_id, const char *treasure_id) {
    char path[MAX_INPUT_SIZE];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    FILE *treasure_file = fopen(path, "rb");
    if (!treasure_file) {
        printf("Error: Could not open hunt '%s'\n", hunt_id);
        return;
    }
    Treasure t;
    int found = 0;
    while (fread(&t, sizeof(Treasure), 1, treasure_file) == 1) {
        if (strcmp(t.id, treasure_id) == 0) {
            found = 1;
            printf("Treasure details:\n");
            printf("ID: %s\n", t.id);
            printf("User: %s\n", t.user_name);
            printf("Location: %.6f, %.6f\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            break;
        }
    }
    fclose(treasure_file);
    if (!found) {
        printf("Error: Treasure '%s' not found in hunt '%s'\n", treasure_id, hunt_id);
    }
    fflush(stdout);
}

// Main
int main() {
    setup_signal_handlers();
    while (running) {
        pause();
    }
    unlink(COMMAND_FILE);
    printf("Monitor stopping...\n");
    usleep(500000);
    return 0;
}
