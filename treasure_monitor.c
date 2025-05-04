#include "treasure.h"//for the treasure structure(header file to have where i need)
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


#define MAX_INPUT_SIZE 512
#define COMMAND_FILE ".monitor_command"
#define RESPONSE_FILE ".monitor_response"

volatile bool running = true;

// Function prototypes
void handle_sigusr1(int sig);
void handle_sigusr2(int sig);
void setup_signal_handlers();
void process_command();
void list_all_hunts(FILE *response);
void list_hunt_treasures(const char *hunt_id, FILE *response);
void view_specific_treasure(const char *hunt_id, const char *treasure_id, FILE *response);

// Signal handler for SIGUSR1 (stop)
void handle_sigusr1(int sig) {
    (void)sig; // Unused parameter
    running = false;
}

// Signal handler for SIGUSR2 (new command)
void handle_sigusr2(int sig) {
    (void)sig; // Unused parameter
    process_command();
}

// Setup signal handlers
void setup_signal_handlers() {
    struct sigaction sa;
    
    // SIGUSR1 handler (stop)
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }
    
    // SIGUSR2 handler (new command)
    sa.sa_handler = handle_sigusr2;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
    
    // Ignore SIGCHLD (not needed in monitor)
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
}

// Process command from hub
void process_command() {
    FILE *cmd_file = fopen(COMMAND_FILE, "r");
    if (!cmd_file) {
        perror("fopen command file");
        return;
    }
    
    char cmd[MAX_INPUT_SIZE];
    if (!fgets(cmd, MAX_INPUT_SIZE, cmd_file)) {
        fclose(cmd_file);
        return;
    }
    fclose(cmd_file);
    
    // Remove newline if present
    cmd[strcspn(cmd, "\n")] = '\0';
    
    FILE *response = fopen(RESPONSE_FILE, "w");
    if (!response) {
        perror("fopen response file");
        return;
    }
    
    // Parse command
    char *space = strchr(cmd, ' ');
    char *arg = NULL;
    if (space) {
        *space = '\0';
        arg = space + 1;
    }
    
    if (strcmp(cmd, "list_hunts") == 0) {
        list_all_hunts(response);
    } else if (strcmp(cmd, "list_treasures") == 0 && arg) {
        list_hunt_treasures(arg, response);
    } else if (strcmp(cmd, "view_treasure") == 0 && arg) {
        // Split arg into hunt_id and treasure_id
        char *treasure_space = strchr(arg, ' ');
        if (treasure_space) {
            *treasure_space = '\0';
            char *treasure_id = treasure_space + 1;
            view_specific_treasure(arg, treasure_id, response);
        } else {
            fprintf(response, "Error: Missing treasure ID\n");
        }
    } else {
        fprintf(response, "Error: Unknown command\n");
    }
    
    fclose(response);
}

// List all hunts (directories in current path)
void list_all_hunts(FILE *response) {
    DIR *dir;
    struct dirent *entry;
    
    fprintf(response, "Available hunts:\n");
    
    dir = opendir(".");
    if (!dir) {
        perror("opendir");
        fprintf(response, "Error: Could not list hunts\n");
        return;
    }
    
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            
            // Count treasures in this hunt
            char path[MAX_INPUT_SIZE];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
            
            struct stat st;
            if (stat(path, &st) == 0) {
                int num_treasures = st.st_size / sizeof(Treasure);
                fprintf(response, "- %s [no of treasures: %d]\n", entry->d_name, num_treasures);
                count++;
            }
        }
    }
    
    closedir(dir);
    
    if (count == 0) {
        fprintf(response, "No hunts found\n");
    }
}

// List all treasures in a hunt
void list_hunt_treasures(const char *hunt_id, FILE *response) {
    char path[MAX_INPUT_SIZE];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    
    FILE *treasure_file = fopen(path, "rb");
    if (!treasure_file) {
        fprintf(response, "Error: Could not open hunt '%s'\n", hunt_id);
        return;
    }
    
    fprintf(response, "Treasures in hunt '%s':\n", hunt_id);
    
    Treasure t;
    while (fread(&t, sizeof(Treasure), 1, treasure_file) == 1) {
        fprintf(response, "- ID: %s, User: %s, Value: %d\n", t.id, t.user_name, t.value);
    }
    
    fclose(treasure_file);
}

// View specific treasure details
void view_specific_treasure(const char *hunt_id, const char *treasure_id, FILE *response) {
    char path[MAX_INPUT_SIZE];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    
    FILE *treasure_file = fopen(path, "rb");
    if (!treasure_file) {
        fprintf(response, "Error: Could not open hunt '%s'\n", hunt_id);
        return;
    }
    
    Treasure t;
    int found = 0;
    
    while (fread(&t, sizeof(Treasure), 1, treasure_file) == 1) {
        if (strcmp(t.id, treasure_id) == 0) {
            found = 1;
            fprintf(response, "Treasure details:\n");
            fprintf(response, "ID: %s\n", t.id);
            fprintf(response, "User: %s\n", t.user_name);
            fprintf(response, "Location: %.6f, %.6f\n", t.latitude, t.longitude);
            fprintf(response, "Clue: %s\n", t.clue);
            fprintf(response, "Value: %d\n", t.value);
            break;
        }
    }
    
    fclose(treasure_file);
    
    if (!found) {
        fprintf(response, "Error: Treasure '%s' not found in hunt '%s'\n", treasure_id, hunt_id);
    }
}

// Main monitor loop
int main() {
    setup_signal_handlers();
    
    
    // Main loop - just wait for signals
    while (running) {
        pause(); // Wait for signals
    }
    
    // Cleanup
    unlink(COMMAND_FILE);
    unlink(RESPONSE_FILE);
    
    printf("Monitor stopping...\n");
    usleep(500000); // Delay as required by spec
    
    return 0;
}