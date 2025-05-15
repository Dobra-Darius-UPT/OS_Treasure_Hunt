#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <limits.h>//for PATH_MAX
#include <stddef.h>
#include "treasure.h" //for the treasure structure(header file to have where i need)

#define LOG_FILE "logged_hunt"
#define TREASURE_FILE "treasures.dat"

// Function prototypes
void print_usage();//in case someone dose not know the functions
int create_hunt_directory(const char* hunt_id);//if the dir for the hunt 
void log_operation(const char* hunt_id, const char* operation); //add the specific message to the log file, also with the time when the operation was performed
void add_treasure(const char* hunt_id);//add a treasure to a specific hunt dir
void list_treasures(const char* hunt_id);
void view_treasure(const char *hunt_id, const char* treasure_id);
void remove_treasure(const char* hunt_id, const char* treasure_id);
void remove_hunt(const char* hunt_id);
int hunt_exists(const char* hunt_id);
void view_log(const char* hunt_id); //added function for a better view of the log-file
void get_treasure_input(Treasure *t);

//-------------------------------------------------------------------------//
//  THE FUNCTION IMPLEMENTATION

void print_usage() {
    printf("Usage: treasure_manager <operation> <hunt_id> [treasure_id]\n");
    printf("Operations:\n");
    printf("  --add                Add a new treasure to the hunt\n");
    printf("  --list               List all treasures in the hunt\n");
    printf("  --view <treasure_id> View details of a specific treasure\n");
    printf("  --remove_treasure <treasure_id> Remove a specific treasure\n");
    printf("  --remove_hunt        Remove an entire hunt\n");
    //for visibility i added a function to print the content of the log in terminal, so i don't have to open the file
    printf("  --view_log          View the operation log for a hunt\n");
}

void view_log(const char *hunt_id) {
    char log_path[PATH_MAX];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);

    FILE *log = fopen(log_path, "r");
    if (!log) {
        printf("No log found for hunt '%s'\n", hunt_id);
        return;
    }

    printf("=== Operation Log for Hunt '%s' ===\n", hunt_id);
    char line[512];
    while (fgets(line, sizeof(line), log)) {
        printf("%s", line);  // Log already contains formatted time
    }
    fclose(log);
}

void get_treasure_input(Treasure *t) {
    printf("Enter Treasure ID: ");
    fgets(t->id, ID_SIZE, stdin);
    t->id[strcspn(t->id, "\n")] = '\0';  // Remove trailing newline

    printf("Enter Username: ");
    fgets(t->user_name, NAME_SIZE, stdin);
    t->user_name[strcspn(t->user_name, "\n")] = '\0';

    printf("Enter Latitude: ");
    while (scanf("%f", &t->latitude) != 1) {
        printf("Invalid input. Enter a number between -90 and 90: ");
        while (getchar() != '\n'); // Clear input buffer
    }

    printf("Enter Longitude: ");
    while (scanf("%f", &t->longitude) != 1) {
        printf("Invalid input. Enter a number between -180 and 180: ");
        while (getchar() != '\n');
    }

    // Clear the input buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Enter Clue: ");
    fgets(t->clue, CLUE_SIZE, stdin);
    t->clue[strcspn(t->clue, "\n")] = '\0';

    printf("Enter Value: ");
    while (scanf("%d", &t->value) != 1) {
        printf("Invalid input. Enter an integer value: ");
        while (getchar() != '\n');
    }
    while ((c = getchar()) != '\n' && c != EOF); // Clear buffer again
}

int create_hunt_directory(const char *hunt_id) {
    struct stat st;
    
    if (stat(hunt_id, &st) == -1) {
        if (mkdir(hunt_id, 0755) == -1) {
            perror("Error creating hunt directory");
            return 0;
        }
        log_operation(hunt_id, "Hunt created");
    }
    return 1;
}

void log_operation(const char *hunt_id, const char *operation) {
    char log_path[PATH_MAX];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);

    FILE *log = fopen(log_path, "a");
    if (!log) {
        perror("Error opening log file");
        return;
    }

    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    //strftime is a function that format the time, in my case, in human readable format

    fprintf(log, "[%s] %s\n", timestamp, operation);  // Write formatted time
    fclose(log);
    
    // Create/update symbolic link
    char symlink_name[PATH_MAX];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    
    // Remove existing symlink if it exists
    unlink(symlink_name);
    
    if (symlink(log_path, symlink_name) == -1) {
        perror("Error creating symbolic link");
    }
}

void add_treasure(const char *hunt_id) {
    Treasure t;
    get_treasure_input(&t);
    
    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, TREASURE_FILE);
    
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }
    
    if (write(fd, &t, sizeof(Treasure)) == -1) {
        perror("Error writing treasure");
    }
    
    close(fd);
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "ADD treasure %s", t.id);
    log_operation(hunt_id, log_msg);
    
    printf("Treasure '%s' added successfully to hunt '%s'\n", t.id, hunt_id);
}

void list_treasures(const char *hunt_id) {
    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, TREASURE_FILE);
    
    struct stat st;
    if (stat(filepath, &st) == -1) {
        printf("No treasures found in hunt '%s'\n", hunt_id);
        return;
    }
    
    printf("\n=== Hunt: %s ===\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));
    
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }
    
    Treasure t;
    printf("\nTreasures:\n");
    printf("ID\t\tUser\t\tValue\tLocation\n");
    printf("------------------------------------------------\n");
    
    while (read(fd, &t, sizeof(Treasure)) > 0) {
        printf("%-12s\t%-12s\t%d\t(%.6f, %.6f)\n", 
               t.id, t.user_name, t.value, t.latitude, t.longitude);
    }
    
    close(fd);
}

void view_treasure(const char *hunt_id, const char *treasure_id) {
    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, TREASURE_FILE);
    
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }
    
    Treasure t;
    int found = 0;
    
    while (read(fd, &t, sizeof(Treasure)) > 0) {
        if (strcmp(t.id, treasure_id) == 0) {
            found = 1;
            printf("\n=== Treasure Details ===\n");
            printf("ID: %s\n", t.id);
            printf("User: %s\n", t.user_name);
            printf("Location: %.6f latitude, %.6f longitude\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            break;
        }
    }
    
    close(fd);
    
    if (!found) {
        printf("Treasure '%s' not found in hunt '%s'\n", treasure_id, hunt_id);
    }
}

void remove_treasure(const char *hunt_id, const char *treasure_id) {
    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, TREASURE_FILE);
    
    char temp_path[PATH_MAX];
    snprintf(temp_path, sizeof(temp_path), "%s/%s.tmp", hunt_id, TREASURE_FILE);
    
    int input_fd = open(filepath, O_RDONLY);
    if (input_fd == -1) {
        perror("Error opening treasure file");
        return;
    }
    
    int output_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("Error creating temporary file");
        close(input_fd);
        return;
    }
    
    Treasure t;
    int found = 0;
    
    while (read(input_fd, &t, sizeof(Treasure)) > 0) {
        if (strcmp(t.id, treasure_id) == 0) {
            found = 1;
            continue; // Skip writing this treasure to the temp file
        }
        write(output_fd, &t, sizeof(Treasure));
    }
    
    close(input_fd);
    close(output_fd);
    
    if (!found) {
        printf("Treasure '%s' not found in hunt '%s'\n", treasure_id, hunt_id);
        unlink(temp_path);
        return;
    }
    
    // Replace original file with temp file
    if (rename(temp_path, filepath) == -1) {
        perror("Error replacing treasure file");
        return;
    }
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "REMOVE treasure %s", treasure_id);
    log_operation(hunt_id, log_msg);
    
    printf("Treasure '%s' removed successfully from hunt '%s'\n", treasure_id, hunt_id);
}

void remove_hunt(const char *hunt_id) {
    char command[PATH_MAX + 10];
    snprintf(command, sizeof(command), "rm -rf %s", hunt_id);
    
    if (system(command) == -1) {
        perror("Error removing hunt directory");
        return;
    }
    
    // Remove the symbolic link
    char symlink_name[PATH_MAX];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);
    
    printf("Hunt '%s' removed successfully\n", hunt_id);
}

int hunt_exists(const char *hunt_id) {
    struct stat st;
    return (stat(hunt_id, &st) == 0 && S_ISDIR(st.st_mode));
}

//-------------------------THE MAIN FUNCTION--------------------//

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    const char *operation = argv[1];
    const char *hunt_id = argv[2];

    if (strcmp(operation, "--add") == 0) {
        if (!create_hunt_directory(hunt_id)) {
            fprintf(stderr, "Failed to create/access hunt directory\n");
            return EXIT_FAILURE;
        }
        add_treasure(hunt_id);
    } 
    else if (strcmp(operation, "--list") == 0) {
        if (!hunt_exists(hunt_id)) {
            fprintf(stderr, "Hunt '%s' does not exist\n", hunt_id);
            return EXIT_FAILURE;
        }
        list_treasures(hunt_id);
    }
    else if (strcmp(operation, "--view") == 0 && argc == 4) {
        if (!hunt_exists(hunt_id)) {
            fprintf(stderr, "Hunt '%s' does not exist\n", hunt_id);
            return EXIT_FAILURE;
        }
        view_treasure(hunt_id, argv[3]);
    }
    else if (strcmp(operation, "--remove_treasure") == 0 && argc == 4) {
        if (!hunt_exists(hunt_id)) {
            fprintf(stderr, "Hunt '%s' does not exist\n", hunt_id);
            return EXIT_FAILURE;
        }
        remove_treasure(hunt_id, argv[3]);
    }
    else if (strcmp(operation, "--remove_hunt") == 0) {
        if (!hunt_exists(hunt_id)) {
            fprintf(stderr, "Hunt '%s' does not exist\n", hunt_id);
            return EXIT_FAILURE;
        }
        remove_hunt(hunt_id);
    }
    else if (strcmp(operation, "--view_log") == 0) {
        if (!hunt_exists(hunt_id)) {
            fprintf(stderr, "Hunt '%s' does not exist\n", hunt_id);
            return EXIT_FAILURE;
        }
        view_log(hunt_id);
    }
    else {
        print_usage();
        return EXIT_FAILURE;
    }

    return 0;
}