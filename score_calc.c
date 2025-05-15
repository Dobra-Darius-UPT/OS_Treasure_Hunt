#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "treasure.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("fopen");
        return 1;
    }

    Treasure t;
    int user_count = 0;
    struct {
        char name[NAME_SIZE];
        int score;
    } users[100];

    while (fread(&t, sizeof(Treasure), 1, file)) {
        int found = 0;
        for (int i = 0; i < user_count; ++i) {
            if (strcmp(users[i].name, t.user_name) == 0) {
                users[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found && user_count < 100) {
            strncpy(users[user_count].name, t.user_name, NAME_SIZE);
            users[user_count].score = t.value;
            ++user_count;
        }
    }
    fclose(file);

    printf("Scores for hunt '%s':\n", argv[1]);
    for (int i = 0; i < user_count; ++i) {
        printf("  %s: %d points\n", users[i].name, users[i].score);
    }

    printf("score_calc done for hunt '%s'\n", argv[1]);
    fflush(stderr);


    //fflush(stdout); // force flush in case output is buffered
    return 0;
}
