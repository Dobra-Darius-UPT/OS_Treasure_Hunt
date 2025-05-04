#ifndef TREASURE_H
#define TREASURE_H

#define ID_SIZE 32
#define NAME_SIZE 64
#define CLUE_SIZE 512

typedef struct {
    char id[ID_SIZE];
    char user_name[NAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

#endif
