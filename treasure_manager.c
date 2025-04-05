#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<sys/stat.h>
#include<string.h>

//i define my sizes for the char* types
#define ID_SIZE 32
#define NAME_SIZE 64
#define CLUE_SIZE 512

//structure for one treasure
typedef struct treasure{
    char ID[ID_SIZE];
    char User_Name[NAME_SIZE];
    float latitude;
    float longitude;
    char Clue[CLUE_SIZE];
    int Value; 
}Treasure;

//a treasure hunt have more treasures(at least one)
//i will ask from input to provide the ID of a hunt
//all hunts are stored in different directories

//function to get the treasure stats
void get_treasure_input(Treasure *t) {
    printf("Enter Treasure ID: ");
    fgets(t->ID, ID_SIZE, stdin);
    strtok(t->ID, "\n");

    printf("Enter Username: ");
    fgets(t->User_Name, NAME_SIZE, stdin);
    strtok(t->User_Name, "\n");

    printf("Enter Latitude: ");
    scanf("%f", &t->latitude);

    printf("Enter Longitude: ");
    scanf("%f", &t->longitude);
    getchar(); //because scanf place a new line token
               //so, if we use fgets after, it will get nothing
    printf("Enter Clue: ");
    fgets(t->Clue, CLUE_SIZE, stdin);
    strtok(t->Clue, "\n");

    printf("Enter Value: ");
    scanf("%d", &t->Value);
    getchar();
}

//function to check if the hunt is existing or not
int check_hunt(const char* hunt_id){
    struct stat st;

    if(stat(hunt_id, &st) == 0){

        if(S_ISDIR(st.st_mode)){//S_ISDIR check if is a directory
            return 1;
        }

    }
    return 0;
}

//function to filter if we need to create a new hunt or not
int filter_hunt(const char* hunt_id){
    if(!check_hunt(hunt_id)){
        printf("Hunt does not exist. Creating directory...\n");
        mkdir(hunt_id, 0755);
    }else{
        printf("Hunt already exists! Adding treasure to it...\n");
    }
}












