#ifndef _BABATYPES_H_
#define _BABATYPES_H_

#include <stdint.h>
#include <stdlib.h>

enum {
    MAX_BOARD_COLS = 35,
    MAX_BOARD_ROWS = 20,
    MAX_BOARD_SIZE = MAX_BOARD_COLS*MAX_BOARD_ROWS,
    MAX_ITEM_TYPES = 26, // one per alphabet letter
    MAX_ITEM_NAME = 31, // name of object
};

typedef enum attr_type {
    ATTR_YOU,    // @
    ATTR_WIN,    // *
    ATTR_STOP,   // .
    ATTR_PUSH,   // +
    ATTR_DEFEAT, // !
    ATTR_HOT,    // (
    ATTR_MELT,   // )
    ATTR_OPEN,   // [
    ATTR_SHUT,   // ]
    ATTR_FLOAT,  // ^
    ATTR_SINK,   // _
    ATTR_MOVE,   // >
    ATTR_TELE,   // <
    ATTR_WEAK,   // ,
    NUM_ATTR_TYPES
} attr_type_t;

typedef enum word_type {
    WORD_IS,   // =
    WORD_AND,  // &
    WORD_HAS,  // ~
    WORD_TEXT, // %
    NUM_WORD_TYPES
} word_type_t;

typedef enum object_type {
    OBJECT_NONE=0, // zero object struct to make empty space
    OBJECT_ITEM,
    OBJECT_ITEM_TEXT,
    OBJECT_ATTR_TEXT,
    OBJECT_WORD_TEXT,
} object_type_t;

typedef enum direction_type {
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_UP
} direction_type_t;

typedef enum input_type {
    INPUT_RIGHT,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_UP,
    INPUT_WAIT,
} input_type_t;

typedef uint8_t item_type_t;

typedef uint16_t object_handle_t; // 0=blank, -1=barrier

typedef struct object {
    uint8_t type;    // actually object_type above
    uint8_t subtype; // either item_type, attr_type or word_type
    uint8_t row;     // grid pos
    uint8_t col;    
    uint8_t facing;  // direction_type
} object_t;

typedef uint32_t item_flags_t;
typedef uint16_t attr_flags_t;

typedef struct item_info {
    char name[MAX_ITEM_NAME];
    char abbrev;
} item_info_t;

typedef struct game_desc {
    size_t rows;
    size_t cols;
    size_t size;
    size_t max_objects;
    size_t num_item_types;
    item_info_t item_info[MAX_ITEM_TYPES];
} game_desc_t;

typedef struct game_state {
    game_desc_t*      desc;    // managed separately
    object_t*         objects; // managed separately
    object_handle_t*  next;    // per-object next pointer for cells
    object_handle_t*  cells;   // per-object 
    attr_flags_t*     attrs;   // attributes for each item and then last one is text
    uint8_t*          xforms;  // transforms for each item and then last one is text
} game_state_t;

object_t* game_objects_alloc(size_t max_objects);

game_state_t* game_state_init(game_desc_t* desc, object_t* objects);

void game_print_rules(const game_state_t* state);

void game_parse(const char* filename,
                game_desc_t** desc,
                object_t** objects);

void game_print(const game_state_t* state); // require no overlapping objects, everything facing right

#endif
