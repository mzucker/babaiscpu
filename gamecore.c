#include "gamecore.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>


#define MAP_OFFSET(row, col, stride) ((row)*(stride) + (col))

const char ATTR_TYPE_CHARS[NUM_ATTR_TYPES+1] = "@*.+!()[]^_><,";
const char KEYWORD_TYPE_CHARS[NUM_KEYWORD_TYPES+1] = "=&~%";

const char* ATTR_NAMES[NUM_ATTR_TYPES] = {
    "YOU", "WIN", "STOP", "PUSH", "DEFEAT", "HOT", "MELT",
    "OPEN", "SHUT", "FLOAT", "SINK", "MOVE", "TELE", "WEAK"
};

const char* KEYWORD_NAMES[NUM_KEYWORD_TYPES] = {
    "IS", "AND", "HAS", "TEXT"
};

//////////////////////////////////////////////////////////////////////

const char* object_str(const game_desc_t* desc,
                       const object_t* object,
                       char buf[MAX_ITEM_NAME+1]) {

    int t = GET_TYPE(object->type_subtype);
    int s = GET_SUBTYPE(object->type_subtype);

    if (t == OBJECT_ITEM) {
        const char* src = desc->item_info[s].name;
        char* dst = buf;
        while (*src) { *dst++ = tolower(*src++); }
        *dst = 0;
    } else if (t == OBJECT_ITEM_TEXT) {
        return desc->item_info[s].name;
    } else if (t == OBJECT_ATTR) {
        return ATTR_NAMES[s];
    } else if (t == OBJECT_KEYWORD) {
        return KEYWORD_NAMES[s];
    } else {
        strcpy(buf, "???");
    }

    return buf;
    
}

//////////////////////////////////////////////////////////////////////

int lookup(const char* str, char c, int maxlen) {
    for (int i=0; i<maxlen; ++i) {
        assert(str[i]);
        if (str[i] == c) { return i; }
    }
    return -1;
}

int lookup_attr_type(char c) {
    return lookup(ATTR_TYPE_CHARS, c, NUM_ATTR_TYPES);
}

int lookup_word_type(char c) {
    return lookup(KEYWORD_TYPE_CHARS, c, NUM_KEYWORD_TYPES);
}

//////////////////////////////////////////////////////////////////////

object_t* game_objects_alloc(size_t max_objects) {
    return (object_t*)calloc(sizeof(object_t), max_objects);
}

void* ffalloc(size_t element_size, size_t count) {
    size_t len = element_size*count;
    void* rval = malloc(len);
    memset(rval, 0xff, len);
    return rval;
}

game_state_t* game_state_alloc(game_desc_t* desc, object_t* objects) {

    game_state_t* rval = malloc(sizeof(game_state_t));

    rval->desc = desc;
    rval->objects = objects;
    rval->next = ffalloc(sizeof(object_index_t), desc->max_objects);
    rval->map = ffalloc(sizeof(object_index_t), desc->map_size);
    rval->attrs = calloc(sizeof(attr_flags_t), desc->num_item_types+1);
    rval->xforms = ffalloc(sizeof(uint8_t), desc->num_item_types);

    return rval;

}

//////////////////////////////////////////////////////////////////////

void parse_nouns(game_state_t* state,
                 size_t row, size_t col,
                 int delta_row, int delta_col,
                 item_flags_t* items,
                 attr_flags_t* attrs) {

    const game_desc_t* desc = state->desc;

    int want_noun = 1;
    
    while (1) {

        row += delta_row;
        col += delta_col;

        if (row >= desc->rows || col >= desc->cols) {
            break;
        }
            
        size_t map_offset = MAP_OFFSET(row, col, desc->cols);

        int hit = 0;

        for (object_index_t oidx=state->map[map_offset];
             oidx!=NULL_OBJECT; oidx = state->next[oidx]) {

            const object_t* object = state->objects + oidx;
            assert(object->row == row && object->col == col);

            int t = GET_TYPE(object->type_subtype);
            int s = GET_SUBTYPE(object->type_subtype);

            if (want_noun) {

                if (t == OBJECT_ITEM_TEXT) {
                    *items |= (1 << s);
                    hit = 1;
                } else if (IS_KEYWORD(object->type_subtype, KEYWORD_TEXT)) {
                    *items |= (1 << desc->num_item_types);
                    hit = 1;
                } else if (attrs && t == OBJECT_ATTR) {
                    *attrs |= (1 << s);
                    hit = 1;
                }
                
            } else if (IS_KEYWORD(object->type_subtype, KEYWORD_AND)) {
                hit = 1;
            }
            
        }

        if (!hit) {
            break;
        }

        want_noun = !want_noun;
        
    }

}

//////////////////////////////////////////////////////////////////////

game_state_t* game_state_init(game_desc_t* desc, object_t* objects) {

    game_state_t* state = game_state_alloc(desc, objects);

    // fill in map
    for (size_t oidx=0; oidx<desc->max_objects; ++oidx) {

        object_t* object = objects + oidx;
        
        if (object->type_subtype) {

            size_t map_offset = MAP_OFFSET(object->row, object->col, desc->cols);
            assert(map_offset < desc->map_size);

            // see what was there before
            object_index_t prev_in_cell = state->map[map_offset];

            state->next[oidx] = prev_in_cell;
            state->map[map_offset] = oidx;
                
        }
        
    }

    // map is initialized, now go and look at words
    for (size_t oidx=0; oidx<desc->max_objects; ++oidx) {
        
        object_t* object = objects + oidx;

        // check for IS
        if (!IS_KEYWORD(object->type_subtype, KEYWORD_IS)) {
            continue;
        }

        // found anchor IS

        int delta_row[2] = { 1, 0 };
        int delta_col[2] = { 0, 1 };
                
        for (int i=0; i<2; ++i) {

            item_flags_t subject_items = 0;
            item_flags_t pred_items = 0;
            attr_flags_t pred_attrs = 0;
                
            parse_nouns(state,
                        object->row, object->col,
                        -delta_row[i], -delta_col[i],
                        &subject_items, NULL);

            if (!subject_items) { continue; }

            parse_nouns(state,
                        object->row, object->col,
                        delta_row[i], delta_col[i],
                        &pred_items, &pred_attrs);

            if (!pred_items && !pred_attrs) { continue; }

            for (size_t sitem=0; sitem<=desc->num_item_types; ++sitem) {
                        
                if (!(subject_items & (1 << sitem))) { continue; }

                state->attrs[sitem] |= pred_attrs;
                
                if (sitem < desc->num_item_types) {
                    for (size_t pitem=0; pitem<desc->num_item_types; ++pitem) {
                        if (pred_items & (1 << pitem)) {
                            if (state->xforms[sitem] == 0xff || pitem == sitem) {
                                state->xforms[sitem] = pitem;
                            }
                        }
                    }
                }

                    
            } // for each item type

        } // for horiz/vert

    } // for each object

    return state;

}

//////////////////////////////////////////////////////////////////////

void game_state_free(game_state_t* state) {
    free(state);
}

//////////////////////////////////////////////////////////////////////

void game_print_rules(const game_state_t* state) {

    const game_desc_t* desc = state->desc;

    for (size_t sitem=0; sitem<=desc->num_item_types; ++sitem) {

        const char* sname;

        if (sitem < desc->num_item_types) {
            sname = desc->item_info[sitem].name;
        } else {
            sname = "TEXT";
        }

        for (size_t attr=0; attr<NUM_ATTR_TYPES; ++attr) {
            if (state->attrs[sitem] & (1 << attr)) {
                printf("%s IS %s\n", sname, ATTR_NAMES[attr]);
            }
        }

        if (sitem < desc->num_item_types) {

            int turn_into = state->xforms[sitem];
            
            if (state->xforms[sitem] < desc->num_item_types) {
                printf("%s IS %s\n", sname, desc->item_info[turn_into].name);
            }


        }
        
    }

}

//////////////////////////////////////////////////////////////////////

void game_parse(const char* filename,
                game_desc_t** pdesc,
                object_t** pobjects) {

    game_desc_t* desc = calloc(sizeof(game_desc_t), 1);

    FILE* istr = fopen(filename, "r");

    int item_lookup[26];
    
    memset(item_lookup, 0xff, sizeof(item_lookup));

    //////////////////////////////////////////////////
    // parse items: letter, space, label, newline
    
    while (1) {
        
        int tok = fgetc(istr);

        if (tok == '\n') {
            break;
        }

        // letter
        assert(isalpha(tok) && islower(tok));

        // space
        assert(fgetc(istr) == ' ');
            
        int item_type = desc->num_item_types++;
        assert(item_type < MAX_ITEM_TYPES); // TODO: throw error

        // set up info
        item_info_t* info = desc->item_info + item_type;
        info->abbrev = tok;

        // parse label
        size_t len=0;

        while (1) {
            tok = fgetc(istr);
            if (tok == '\n') { break; }
            assert(isalpha(tok));
            info->name[len++] = toupper(tok);
            assert(len < MAX_ITEM_NAME);
        }

        assert(len > 0);

        // null-terminate
        info->name[len] = 0;

        printf("item type %d has abbrev %c and name '%s'\n",
               item_type, info->abbrev, info->name);

        // set item lookup by letter
        item_lookup[info->abbrev-'a'] = item_type;

    }

    //////////////////////////////////////////////////
    // read rest of file into array of chars

    size_t padded_cols = 0;

    int tok;

    // parse a row of hashmarks to get width
    while ( (tok = fgetc(istr)) == '#' ) {
        ++padded_cols;
    }
    assert(tok == '\n');
    printf("padded width = %d\n", (int)padded_cols);

    desc->cols = padded_cols - 2;
    desc->rows = 0;

    char cgrid[MAX_BOARD_SIZE];
    size_t cgrid_offset = 0;
    
    int final_row = 0;
    
    while (!final_row) {
        
        for (size_t j=0; j<padded_cols; ++j) {
            
            tok = fgetc(istr);
            
            assert(tok > 0 && tok != '\n');
            
            if (j == 0 || j+1 == padded_cols || final_row) {
                assert(tok == '#');
                continue;
            }
            
            if (j == 1 && tok == '#') {
                final_row = 1;
                continue;
            }
            
            cgrid[cgrid_offset++] = tok;
            if (tok != ' ') {
                ++desc->max_objects;
            }
                
        }

        tok = fgetc(istr);
        assert(tok == '\n');
        if (!final_row) { ++desc->rows; }
        
    };

    tok = fgetc(istr);
    assert(tok == -1);

    //////////////////////////////////////////////////
    // read array of chars into object array
    
    desc->map_size = desc->rows * desc->cols;

    printf("board is %d rows x %d cols with %d max objects and %d item types\n",
           (int)desc->rows, (int)desc->cols,
           (int)desc->max_objects, (int)desc->num_item_types);
           
    cgrid_offset = 0;
    size_t object_idx = 0;

    object_t* objects = game_objects_alloc(desc->max_objects);

    for (size_t i=0; i<desc->rows; ++i) {
        for (size_t j=0; j<desc->cols; ++j, ++cgrid_offset) {
            
            char c = cgrid[cgrid_offset];
            
            if (c == ' ') {
                continue;
            }

            object_t* object = objects + object_idx++;

            object->row = i;
            object->col = j;
            
            if (isalpha(c)) {
                
                int cidx = tolower(c) - 'a';
                int item_type = item_lookup[cidx];
                
                assert(item_type < desc->num_item_types);

                if (isupper(c)) {
                    object->type_subtype = MAKE_TYPE(OBJECT_ITEM_TEXT, item_type);
                } else {
                    object->type_subtype = MAKE_TYPE(OBJECT_ITEM, item_type);
                }

                continue;
                
            } 

            int attr_type = lookup_attr_type(c);
            
            if (attr_type != -1) {

                object->type_subtype = MAKE_TYPE(OBJECT_ATTR, attr_type);
                continue;
                    
            }

            int word_type = lookup_word_type(c);
                    
            if (word_type != -1) {

                object->type_subtype = MAKE_TYPE(OBJECT_KEYWORD, word_type);
                continue;

            }

            assert( 0 && "unexpected character in input!" );

        }

    }

    *pdesc = desc;
    *pobjects = objects;

    
}

// assert no overlapping objects, everything facing right
void game_print(const game_state_t* state) {

    const game_desc_t* desc = state->desc;

    size_t map_offset = 0;

    for (size_t j=0; j<desc->cols+2; ++j) {
        printf("#");
    }
    printf("\n");
    
    for (size_t i=0; i<desc->rows; ++i) {

        printf("#");
        
        for (size_t j=0; j<desc->cols; ++j, ++map_offset) {

            object_index_t oidx = state->map[map_offset];

            if (oidx == NULL_OBJECT) {
                
                printf(" ");
                
            } else {
                
                const object_t* object = state->objects + oidx;

                int t = GET_TYPE(object->type_subtype);
                int s = GET_SUBTYPE(object->type_subtype);

                char ochar = '?';

                if (t == OBJECT_NONE) {
                    ochar = ' ';
                } else if (t == OBJECT_ITEM) {
                    ochar = desc->item_info[s].abbrev;
                } else if (t == OBJECT_ITEM_TEXT) {
                    ochar = toupper(desc->item_info[s].abbrev);
                } else if (t == OBJECT_ATTR) {
                    ochar = ATTR_TYPE_CHARS[s];
                } else {
                    assert(t == OBJECT_KEYWORD);
                    ochar = KEYWORD_TYPE_CHARS[s];
                }
                
                printf("%c", ochar);
                
            }
            
        }

        printf("#\n");
        
    }

    for (size_t j=0; j<desc->cols+2; ++j) {
        printf("#");
    }
    printf("\n");
    
}
