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

#define ROOMS_LN 10
#define ROOMS_N 7
#define ROOMS_T 3
#define CONNECT_MAX 6
#define CONNECT_MIN 3

struct room{
    char* type;
    char* name;
    int connect_n;
    struct room* connections[CONNECT_MAX];
};


struct room* rooms[ROOMS_N];
int room_bool_n[ROOMS_LN];          // bool when gen rooms for if room used
int room_bool_t[ROOMS_T];
char* room_names[ROOMS_LN] = { "Eastmar", "Falkreat", "Haafing", "Hjaalm", "ThePale", "TheReach", "TheRift", "Whiterun", "Wintold", "Oblivion" };
char* room_types[ROOMS_T] = { "START_ROOM", "END_ROOM", "MID_ROOM" };
char dir_name[11] = "reedz.rooms";  // name of directory without pID
char* dir_str;                      // stores dir_name + pID
int dir_str_len = 12;               // initial length, will be updated with pID


/*****************************************************
 ** Generate Initializer
 ** Preconditions: room_bool_t, room_bool_n, and rooms are NULL
 ** Postconditions: all arrays are initialized
 *****************************************************/
void gen_initializer() {
    // initialize room bool to false
    int i; for ( i = 0; i < ROOMS_T; i++) {
        room_bool_t[i] = -1;
    }

    // initialize room bool to false
    int j; for ( j = 0; j < ROOMS_LN; j++) {
        room_bool_n[j] = 0;
    }

    // initialize room array with empty room
    int k; for ( k = 0; k < ROOMS_N; k++) {
        rooms[k] = (struct room *)malloc(sizeof(struct room));
        rooms[k]->connect_n = 0;
    }
}

/*****************************************************
 ** Make Room File
 ** Preconditions: dir_str = name + pID, dir_str_len = 11 + pID
 ** Postconditions: file has been written with information.
 *****************************************************/
void make_room_file(struct room* roomf) {
    // allocate space for file name
    int room_name_len = strlen(roomf->name);
    room_name_len += 5; // for "_room"
    char* room_dir = (char *)malloc(sizeof(char) * (dir_str_len + room_name_len));

    // copy directory and name for file
    strcpy(room_dir, dir_str);
    strcat(room_dir, "/");
    strcat(room_dir, roomf->name);
    strcat(room_dir, "_room");

    // write file room contents
    FILE *room_file = fopen(room_dir, "w+");
    fprintf(room_file, "ROOM NAME: %s\n", roomf->name);
    int i; for ( i = 0; i < roomf->connect_n; i++) {
        fprintf(room_file, "CONNECTION %d: %s\n", i + 1, roomf->connections[i]->name);
    }
    fprintf(room_file, "ROOM TYPE: %s\n", roomf->type);
    fclose(room_file);
}

/*****************************************************
 ** Make Directory
 ** Preconditions: dir_str = null, dir_str_len = 11, no other functions are called in
    main before make_dir()
 ** Postconditions: dir_str = reedz.rooms. + pID, dir_str_len += pID length
 *****************************************************/
void make_dir() {
    int pid = getpid();
    int pid_len = 5;
    if (pid > 9999) {
        pid_len = 6;
    }
    // Write formatted data to string
    char* pid_str = (char *)malloc(sizeof(char) * pid_len);
    sprintf(pid_str, ".%d", pid);

    // Add pID string to directory name
    dir_str = (char *)malloc(sizeof(char) * (dir_str_len + pid_len));
    strcat(dir_str, dir_name);
    strcat(dir_str, pid_str);
    dir_str_len += pid_len;
    // make directory (string, chmod mode)
    int result = mkdir(dir_str, 0755);
}

/*****************************************************
 ** Generate Room Names
 ** Preconditions: room_bool_n is uninitialized
 ** Postconditions: rooms->names added, room_bool_n[0-max] = 1
 *****************************************************/
void gen_room_names() {
    // Generate rooms with names
    int idx = 0;
    int r;
    while (idx < ROOMS_N) {
        r = rand() % ROOMS_LN;
        // if random room name hasn't been used
        if (room_bool_n[r] == 0) {
            rooms[idx]->name = room_names[r];
            room_bool_n[r] = 1;
            idx++;
        }
    }
}

/*****************************************************
 ** Generate Room Types
 ** Preconditions: room_bool_n is uninitialized
 ** Postconditions: rooms->names added, room_bool_n[0-max] = 1
 *****************************************************/
void gen_room_types() {
    // Generate rooms with types
    int idx = 0;
    int start;
    int end;
    // Generate Start Module
    start = rand() % ROOMS_N;
    rooms[start]->type = room_types[0];

    // Generate End Module
    end = rand() % ROOMS_N;
    while(1) {
        if (end != start) {
            rooms[end]->type = room_types[1];
            break;
        }
        end = rand() % ROOMS_N;
    }

    // Generate rest of Middle Modules
    while (idx < ROOMS_N) {
        if (idx != start && idx != end) {
            rooms[idx]->type = room_types[2];
        }
        idx++;
    }
}

/*****************************************************
 ** Is Graph Full
 ** Preconditions:
 ** Postconditions: returns 0 or 1 if there are at least 3
    outbound connections to each room
 *****************************************************/
int is_graph_full() {
    int i; for ( i = 0; i < ROOMS_N; i++) {
        if(rooms[i]->connect_n < CONNECT_MIN) {
            return 0;
        }
    }
    return 1;
}

/*****************************************************
 ** Connection already exists
 ** Preconditions: Rooms a and b are constructed
 ** Postconditions: returns 1 if other room is in
    room connection array
 *****************************************************/
int connect_already_exists(struct room* a, struct room* b) {
    int i; for ( i = 0; i < CONNECT_MAX; i++) {
        if (a->connections[i] == b || b->connections[i] == a) {
            return 1;
        }
    }
    return 0;
}

/*****************************************************
 ** Connect Rooms
 ** Preconditions: a and b are constructed and not already connected
 ** Postconditions: a and b added to each room connection array
 *****************************************************/
void connect_rooms(struct room* a, struct room* b) {
    int a_idx = a->connect_n;
    int b_idx = b->connect_n;
    a->connections[a_idx] = b;
    b->connections[b_idx] = a;
    a->connect_n++;
    b->connect_n++;
}

/*****************************************************
 ** Generate Room Connections
 ** Preconditions: graph is not already full
 ** Postconditions: graph is full
 *****************************************************/
void gen_room_connections() {
    int rc1, rc2;
    while(is_graph_full() == 0) {
        rc1 = rand() % ROOMS_N;
        rc2 = rand() % ROOMS_N;

        // if not same index --------------------
        if (rc1 != rc2) {
            // if room connect 1 in range -------
            if (rooms[rc1]->connect_n < CONNECT_MAX) {
                //if room connect 2 in range ----
                if (rooms[rc2]->connect_n < CONNECT_MAX) {
                    if ((connect_already_exists(rooms[rc1], rooms[rc2])) == 0) {
                        connect_rooms(rooms[rc1], rooms[rc2]);
                    }
                }
            }
        }
    }
}

/*****************************************************
 ** Generate Room Files
 ** Preconditions: room name, type, and connections have been generated
 ** Postconditions: files have been added to reedz.rooms.pid
 *****************************************************/
void gen_room_files() {
    int i; for ( i = 0; i < ROOMS_N; i++) {
        make_room_file(rooms[i]);
    }
}

int main() {
    // seed random generator
    time_t t;
    srand((unsigned)time(&t));

    // make directory and files -----------
    make_dir();
    gen_initializer();
    gen_room_names();
    gen_room_types();
    gen_room_connections();
    gen_room_files();
    return 0;
}
