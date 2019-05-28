#include "gamecore.h"
#include <stdio.h>

int main(int argc, char** argv) {

    if (argc != 2) {
        fprintf(stderr, "usage: babaiscpu LEVEL.txt\n");
        exit(1);
    }

    const char* infile = argv[1];

    game_desc_t* desc;
    object_t* objects;

    game_parse(infile, &desc, &objects);

    game_state_t* state = game_state_init(desc, objects);

    game_print_rules(state);
    game_print(state);
    free(state);

    free(desc);
    free(objects);

    return 0;
    
}
