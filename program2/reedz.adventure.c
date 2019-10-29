/***************************************
 ** Author: Zach Reed
 ** Description: Build Rooms for Adventure
 ** Date: 5/8/2019
 ****************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>

#define ROOMS_LN 10
#define ROOMS_N 7
#define ROOMS_T 3
#define CONNECT_MAX 6
#define CONNECT_MIN 3
#define ROOM_NAME_PREFIX 11
#define CONNECT_PREFIX 11

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread;

struct room{
    char* type;
    char* name;
    int connect_n;
    struct room* connections[CONNECT_MAX];
};

char working_dir[256];
struct stat* file;
struct room* rooms[ROOMS_N];
struct room* g_rooms[100];
int rn_steps;
int rooms_idx;                  // used for adding rooms
char* gtime;
char* room_names[ROOMS_LN] = { "Eastmar", "Falkreat", "Haafing", "Hjaalm", "ThePale", "TheReach", "TheRift", "Whiterun", "Wintold", "Oblivion" };
char* room_types[ROOMS_T] = { "START_ROOM", "END_ROOM", "MID_ROOM" };

/*****************************************************
 ** Constructor
 ** Preconditions:  rooms is NULL
 ** Postconditions: array is initiallized
 *****************************************************/
void constructor() {
    memset(working_dir, '\0', sizeof(working_dir)); // set block of memory to value

    rn_steps = 0;
    int k;
    for ( k = 0; k < ROOMS_N; k++) {
        rooms[k] = (struct room *)malloc(sizeof(struct room));
        rooms[k]->connect_n = 0;
    }
}

/*****************************************************
 ** Deconstructor
 ** Preconditions:  rooms is not NULL
 ** Postconditions: rooms is NULL
 *****************************************************/
void deconstructor() {
    int k;
    for (k = 0; k < ROOMS_N; k++) {
        free(rooms[k]);
    }
}

/*****************************************************
 ** Is Room File
 ** Preconditions: read_directory() has been called
 ** Postconditions: returns true if room name has prefix in room_names
 *****************************************************/
int is_room(char *file_name) {
    int i;
    for (i = 0; i < ROOMS_LN; i++) {
        if(strstr(file_name, room_names[i])) {
            return i;
        }
    }
    return -1;
}

/*****************************************************
 ** Is Room in Array
 ** Preconditions: read_directory() has been called
 ** Postconditions: returns true if room name has prefix in room_names
 *****************************************************/
int is_room_arr(char *file_name) {
    int i;
    for (i = 0; i < ROOMS_N; i++) {
        if(strstr(file_name, rooms[i]->name)) {
            return i;
        }
    }
    return -1;
}
/*****************************************************
 ** Is Room Connected
 ** Preconditions: con_idx is the index of a room in room_names
 ** Postconditions: returns true if room name is connection name
 *****************************************************/
int is_room_connect(struct room* r, int con_idx) {
    int i;
    for (i = 0; i < r->connect_n; i++) {
        if (r->connections[i]->name == rooms[con_idx]->name) {
            return i;
        }
    }
    return -1;
}

/*****************************************************
 ** Is Type
 ** Preconditions: read_directory() has been called
 ** Postconditions: returns index if type is in room_types[]
 *****************************************************/
int is_type(char *file_name) {
    int i;
    file_name += CONNECT_PREFIX;
    for (i = 0; i < ROOMS_T; i++) {
        if (strstr(file_name, room_types[i])) {
            return i;
        }
    }
    return -1;
}

/*****************************************************
 ** Get Room
 ** Preconditions: read_directory() has been called
 ** Postconditions: returns room of name shared by room_name[rn_idx]
 *****************************************************/
struct room* get_room(int rn_idx) {
    int i;
    for (i = 0; i < ROOMS_N; i++) {
        if(rooms[i]->name == room_names[rn_idx]) {
            return rooms[i];
        }
    }
    return NULL;
}

/*****************************************************
 ** Add Room
 ** Preconditions: is_room() returned true
 ** Postconditions: room added to room array
 *****************************************************/
void add_room(char *file_name, int nidx) {
    FILE *room = fopen(file_name, "r");
    rooms[rooms_idx]->name = room_names[nidx];
    char room_line[100];
    fgets(room_line, 100, room);
    while (strstr(fgets(room_line, 100, room), "CONNECTION ")) {
        rooms[rooms_idx]->connect_n++;
        memset(room_line, '\0', sizeof(room_line));
    }

    // get type from file
    int tidx = is_type(room_line);
    if (tidx > -1) {
        rooms[rooms_idx]->type = room_types[tidx];
    }

    fclose(room);
    rooms_idx++;
}

/*****************************************************
 ** Add Room Connections
 ** Preconditions: is_room() returned true
 ** Postconditions: room added to room array connections[]
 *****************************************************/
void add_room_connections(char *file_name, int nidx, int ridx) {
    FILE *room = fopen(file_name, "r");
    char room_line[256];
    fgets(room_line, 256, room);
    int rnidx;
    int cidx = 0;
    while (strstr(fgets(room_line, 256, room), "CONNECTION ")) {
        rnidx = is_room(room_line);
        if (ridx > -1) {
            rooms[ridx]->connections[cidx] = get_room(rnidx);
            cidx++;
        }
    }
}

/*****************************************************
 ** Get Rooms
 ** Preconditions: get_directory() called
 ** Postconditions: reads rooms from directory for writing
 *****************************************************/
void get_rooms(char* newest_dir) {
  // dirent = directory stream
    struct dirent* rooms_in_dir;
    DIR* rooms_to_check = opendir(newest_dir);
    char new_file[256];
    int index;

    if (rooms_to_check > 0) {
        while ((rooms_in_dir = readdir(rooms_to_check))) {
            index = is_room(rooms_in_dir->d_name);
            if (index != -1) {
                // clear string and add file path
                memset(new_file, '\0', sizeof(new_file));
                strcpy(new_file, newest_dir);
                strcat(new_file, "/");
                strcat(new_file, rooms_in_dir->d_name);
                add_room(new_file, index);
            }
        }
    }
}

/*****************************************************
 ** Get Room Connections
 ** Preconditions: get_directory() called
 ** Postconditions: reads rooms from directory for writing
 *****************************************************/
void get_room_connections(char *newest_dir) {
    struct dirent* rooms_in_dir;
    DIR* rooms_to_check = opendir(newest_dir);
    char new_file[256];
    int index;
    int r_idx = 0;
    if (rooms_to_check > 0) {
        while ((rooms_in_dir = readdir(rooms_to_check))) {
            index = is_room(rooms_in_dir->d_name);
            if (index != -1) {
                // clear string and add file path
                memset(new_file, '\0', sizeof(new_file));
                strcpy(new_file, newest_dir);
                strcat(new_file, "/");
                strcat(new_file, rooms_in_dir->d_name);
                add_room_connections(new_file, index, r_idx);
                r_idx++;
            }
        }
    }
}

/*****************************************************
 ** Get Room Connections
 ** Preconditions: rooms[] has been generated from file
 ** Postconditions: initializes game rooms[] with START ROOM
 *****************************************************/
void g_room_gen() {
    int i;
    for (i = 0; i < ROOMS_N; i++) {
        // if room is START_ROOM
        if (rooms[i]->type == room_types[0]) {
            g_rooms[0] = rooms[i];
        }
    }
}

/*****************************************************
 ** Get Time
 ** Preconditions:
 ** Postconditions: gtime is updated with current time
 *****************************************************/
void get_time() {
    //size_t wdlen = sizeof(working_dir);
    //size_t ctlen = 16;
    //char* time_file = (char *) malloc(sizeof(char) * (wdlen + ctlen));
    //sprintf(time_file, "%s/currentTime.txt", working_dir);
    FILE *file = fopen("currentTime.txt", "r");
    char line[256];
    char line_cpy[256];
    memset(line, '\0', 256);
    memset(line_cpy, '\0', 256);
    if (file) {
        while (fgets(line, 256, file)) {
            strcpy(line_cpy, line);
        }
        printf("\n%s", line_cpy);

    }
    fclose(file);
}

/*****************************************************
 ** Write Time
 ** Preconditions: none
 ** Postconditions: currentTime.txt in . directory file appended with time if exists
 *****************************************************/
void* write_time(void* args) {
    pthread_mutex_lock(&mutex);

    char str[80];
    size_t maxsize = 256;
    time_t rawtime;
    const struct tm *time_p;

    time(&rawtime);
    time_p = localtime(&rawtime);

    strftime(str, maxsize, "%I:%M%p, %A, %B %d, %Y\n", time_p);
    FILE *file = fopen("currentTime.txt", "a");
    fputs(str, file);
    fclose(file);

    pthread_mutex_unlock(&mutex);
    return NULL;
}


/*****************************************************
 ** Play Game
 ** Preconditions: get_rooms() and get_connections() has been called
 ** Postconditions: game exits
 *****************************************************/
void play_game() {

    // Initialize getline variables
    char *line;
    size_t line_size = 256;
    size_t buff_char;
    line = (char *)malloc(sizeof(char) * line_size);

    // prepare game array
    g_room_gen();
    struct room* current_room;
    current_room = g_rooms[0];
    int idx = 1;
    int gbool = 0;

    // while game is being played ---------------------------------------------
    while(gbool == 0) {
        // if current room is end room ----------------------------
        if(current_room->type == room_types[1]) {
            printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
            printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", rn_steps);
            int j;
            for (j = 1; j < rn_steps; j++) {
                printf("%s\n", g_rooms[j]->name);
            }
            printf("%s\n", current_room->name);
            gbool = 1;
        }
        else {
            // print current room name ----------------------------
            printf("\nCURRENT LOCATION: %s\n", current_room->name);
            int i;
            // print connections ----------------------------------
             printf("POSSIBLE CONNECTIONS: ");
            for (i = 0; i < (current_room->connect_n - 1); i++) {
                printf("%s, ", current_room->connections[i]->name);
            }
            printf("%s.", current_room->connections[i++]->name);

            // Where To? loop -------------------------------------
            while(1) {
                printf("\nWHERE TO? >");
                buff_char = getline(&line, &line_size, stdin);

                int is_r = is_room_arr(line);
                if (is_r != -1) {
                    int is_rc = is_room_connect(current_room, is_r);

                    if(is_rc != -1) {
                        current_room = rooms[is_r];
                        g_rooms[idx] = current_room;
                        idx++;
                        rn_steps++;
                        break;
                    }
                    else {
                        printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
                        break;
                    }
                }
                else {
                    if (strstr(line, "time")) {
                        pthread_create(&thread, NULL, write_time, NULL);
                        pthread_join(&thread, NULL);
                        get_time();
                    }
                    else {
                        printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
                        break;
                    }
                }
            }
        }
    }
    free(line);
}

int main() {
    constructor();
    rooms_idx = 0;

    char prefix_dir[32] = "reedz.rooms.";
    char newest_dir[256];     // holds newer time directory
    int newest_dir_time = -1;
    memset(newest_dir, '\0', sizeof(newest_dir));
    DIR* dir_to_check;
    struct dirent* file_in_dir;
    struct stat dir_attr;

    dir_to_check = opendir(".");

    // if directory opens
    if (dir_to_check > 0) {
        // while room files in directory
        while ((file_in_dir = readdir(dir_to_check)) != NULL) {
            // if file name = prefix directory characters
            if(strstr(file_in_dir->d_name, prefix_dir)) {
                // get directory attributes stat = file status
                stat(file_in_dir->d_name, &dir_attr);

                // if directory time is newer than stored time
                if ((int)dir_attr.st_mtime > newest_dir_time) {
                    newest_dir_time = (int)dir_attr.st_mtime;
                    memset(newest_dir, '\0', sizeof(newest_dir));
                    strcpy(newest_dir, file_in_dir->d_name);
                }
            }
        }
        strcpy(working_dir, newest_dir);
        get_rooms(newest_dir);
        get_room_connections(newest_dir);
        play_game();
    }
    deconstructor();
    return 0;
}
