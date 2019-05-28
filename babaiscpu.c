#include "gamecore.h"
#include <stdio.h>

int main(int argc, char** argv) {

    if (argc != 2) {
        fprintf(stderr, "usage: babaiscpu LEVEL.txt\n");
        exit(1);
    }

    printf("sizeof(object_t) = %d\n", (int)sizeof(object_t));

    const char* infile = argv[1];

    game_desc_t* desc;
    object_t* objects;

    game_state_t state;

    game_parse(infile, &desc, &objects);

    game_state_init(&state, desc, objects);
    game_print_rules(&state);
    game_print(&state);

    free(desc);
    free(objects);

    return 0;
    
}
