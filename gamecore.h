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

    TYPE_BITS = 4,
    TYPE_MASK = (1 << TYPE_BITS)-1,

    SUBTYPE_BITS = 4,
    SUBTYPE_MASK = (1 << SUBTYPE_BITS)-1,

    
};

#define MAKE_TYPE(type, subtype) ((((type) & TYPE_MASK) << SUBTYPE_BITS) | ((subtype) & SUBTYPE_MASK))

#define GET_TYPE(type_subtype) (((type_subtype) >> SUBTYPE_BITS) & TYPE_MASK)
#define GET_SUBTYPE(type_subtype) ((type_subtype) & SUBTYPE_MASK)
#define MAKE_KEYWORD(kw) MAKE_TYPE(OBJECT_KEYWORD, kw)
#define IS_KEYWORD(type_subtype, kw) ((type_subtype) == MAKE_KEYWORD(kw))


typedef enum object_type {
    OBJECT_NONE=0, // zero object struct to make empty space
    OBJECT_ITEM,
    OBJECT_ITEM_TEXT,
    OBJECT_ATTR,
    OBJECT_KEYWORD,
} object_type_t;

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
    NUM_ATTR_TYPES // 
} attr_type_t;

typedef enum keyword_type {
    KEYWORD_IS,   // =
    KEYWORD_AND,  // &
    KEYWORD_HAS,  // ~
    KEYWORD_TEXT, // %
    NUM_KEYWORD_TYPES
} keyword_type_t;


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

typedef uint16_t object_index_t; // 0=blank, -1=barrier

enum {
    NULL_OBJECT = (object_index_t)-1
};

typedef struct object {
    uint8_t type_subtype;
    //uint8_t type;    // actually object_type above
    //uint8_t subtype; // either item_type, attr_type or word_type
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
    size_t map_size;
    size_t max_objects;
    size_t num_item_types;
    item_info_t item_info[MAX_ITEM_TYPES];
} game_desc_t;

typedef struct game_state {
    game_desc_t*    desc;    // managed separately
    object_t*       objects; // managed separately
    object_index_t* next;    // per-object next pointer for cells
    object_index_t* map;   // per-cell object handle for map
    attr_flags_t*   attrs;   // attributes for each item and then last one is text
    uint8_t*        xforms;  // transforms for each item 
} game_state_t;

object_t* game_objects_alloc(size_t max_objects);

game_state_t* game_state_init(game_desc_t* desc, object_t* objects);

void game_print_rules(const game_state_t* state);

void game_parse(const char* filename,
                game_desc_t** desc,
                object_t** objects);

void game_print(const game_state_t* state); // require no overlapping objects, everything facing right

#endif
