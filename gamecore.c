#include "gamecore.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define GSZ sizeof(game_state_t)

const char ATTR_TYPE_CHARS[NUM_ATTR_TYPES+1] = "@*.+!()[]^_><,";
const char WORD_TYPE_CHARS[NUM_WORD_TYPES+1] = "=&~%";

const uint8_t NULL_XFORM = (uint8_t)-1;

const char* ATTR_NAMES[NUM_ATTR_TYPES] = {
    "YOU", "WIN", "STOP", "PUSH", "DEFEAT", "HOT", "MELT",
    "OPEN", "SHUT", "FLOAT", "SINK", "MOVE", "TELE", "WEAK"
};

const char* WORD_NAMES[NUM_WORD_TYPES] = {
    "IS", "AND", "HAS", "TEXT"
};

const char* object_str(const game_desc_t* desc,
                       const object_t* object,
                       char buf[MAX_ITEM_NAME+1]) {

    if (object->type == OBJECT_ITEM) {
        const char* src = desc->item_info[object->subtype].name;
        char* dst = buf;
        while (*src) { *dst++ = tolower(*src++); }
        *dst = 0;
    } else if (object->type == OBJECT_ITEM_TEXT) {
        return desc->item_info[object->subtype].name;
    } else if (object->type == OBJECT_ATTR_TEXT) {
        return ATTR_NAMES[object->subtype];
    } else if (object->type == OBJECT_WORD_TEXT) {
        return WORD_NAMES[object->subtype];
    } else {
        strcpy(buf, "???");
    }

    return buf;
    
}

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
    return lookup(WORD_TYPE_CHARS, c, NUM_WORD_TYPES);
}

object_t* game_objects_alloc(size_t max_objects) {
    return (object_t*)calloc(sizeof(object_t), max_objects);
}

game_state_t* game_state_alloc(game_desc_t* desc, object_t* objects) {

    game_state_t* rval = malloc(sizeof(game_state_t));

    rval->desc = desc;
    rval->objects = objects;
    rval->next = calloc(sizeof(object_handle_t), desc->max_objects);
    rval->cells = calloc(sizeof(object_handle_t), desc->size);
    rval->attrs = calloc(sizeof(attr_flags_t), desc->num_item_types+1);
    rval->xforms = calloc(sizeof(uint8_t), desc->num_item_types);

    memset(rval->xforms, NULL_XFORM, desc->num_item_types);

    return rval;

}

void parse_np(game_state_t* state,
              size_t row, size_t col,
              int delta_row, int delta_col,
              item_flags_t* items,
              attr_flags_t* attrs) {

    const game_desc_t* desc = state->desc;

    int want_noun = 1;
    
    while (1) {

        row += delta_row;
        col += delta_col;

        size_t map_offset = col + row*desc->cols;

        int hit = 0;

        for (object_handle_t oidx=state->cells[map_offset];
             oidx!=0; oidx = state->next[oidx]) {

            const object_t* object = state->objects + oidx;
            assert(object->row == row && object->col == col);

            if (want_noun) {

                if (object->type == OBJECT_ITEM_TEXT) {
                    *items |= (1 << object->subtype);
                    hit = 1;
                } else if (object->type == OBJECT_WORD_TEXT && object->subtype == WORD_TEXT) {
                    *items |= (1 << desc->num_item_types);
                    hit = 1;
                } else if (attrs && object->type == OBJECT_ATTR_TEXT) {
                    *attrs |= (1 << object->subtype);
                    hit = 1;
                }
                
            } else if (object->type == OBJECT_WORD_TEXT && object->subtype == WORD_AND) {
                hit = 1;
            }

            //char buf[64];
            //printf("parse_np found %s at (%d, %d), hit=%d\n",
            //object_str(desc, object, buf), (int)row, (int)col, hit);
            
        }

        if (!hit) {
            break;
        }

        want_noun = !want_noun;
        
    }

}

game_state_t* game_state_init(game_desc_t* desc, object_t* objects) {

    game_state_t* state = game_state_alloc(desc, objects);

    assert(objects[0].type == OBJECT_NONE);

    for (size_t oidx=1; oidx<desc->max_objects; ++oidx) {

        object_t* object = objects + oidx;
        
        if (object->type != OBJECT_NONE) {

            // always barriers on edge
            assert(object->col > 0 && object->col < desc->cols - 1);
            assert(object->row > 0 && object->row < desc->rows - 1);
                
            size_t map_offset = object->col + object->row * desc->cols;
            assert(map_offset < desc->size);

            // see what was there before
            object_handle_t prev_in_cell = state->cells[map_offset];

            state->next[oidx] = prev_in_cell;
            state->cells[map_offset] = oidx;
                
        }
        
    }

    // map is initialized, now go and look at words
    for (size_t oidx=1; oidx<desc->max_objects; ++oidx) {
        
        object_t* object = objects + oidx;

        // check for IS
        if (object->type != OBJECT_WORD_TEXT || object->subtype != WORD_IS) {
            continue;
        }

        // found anchor IS

        int delta_row[2] = { 1, 0 };
        int delta_col[2] = { 0, 1 };
                
        for (int i=0; i<2; ++i) {

            item_flags_t subject_items = 0;
            item_flags_t pred_items = 0;
            attr_flags_t pred_attrs = 0;
                
            parse_np(state,
                     object->row, object->col,
                     -delta_row[i], -delta_col[i],
                     &subject_items, NULL);

            if (!subject_items) { continue; }

            parse_np(state,
                     object->row, object->col,
                     delta_row[i], delta_col[i],
                     &pred_items, &pred_attrs);

            if (!pred_items && !pred_attrs) { continue; }

            //printf("parser found %d|%d|%d\n", subject_items, pred_items, pred_attrs);
            
            for (size_t sitem=0; sitem<=desc->num_item_types; ++sitem) {
                        
                if (!(subject_items & (1 << sitem))) { continue; }

                state->attrs[sitem] |= pred_attrs;
                
                if (sitem < desc->num_item_types) {
                    for (size_t pitem=0; pitem<desc->num_item_types; ++pitem) {
                        if (pred_items & (1 << pitem)) {
                            if (state->xforms[sitem] == NULL_XFORM || pitem == sitem) {
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

void game_state_free(game_state_t* state) {
    free(state);
}

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
            
            size_t turn_into = state->xforms[sitem];
        
            if (turn_into != NULL_XFORM) {
                printf("%s IS %s\n", sname, desc->item_info[turn_into].name);
            }


        }
        
    }

}


void game_parse(const char* filename,
                game_desc_t** pdesc,
                object_t** pobjects) {

    game_desc_t* desc = calloc(sizeof(game_desc_t), 1);

    FILE* istr = fopen(filename, "r");

    int item_lookup[26];
    
    for (int i=0; i<26; ++i) { item_lookup[i] = -1; }

    // part 1: parse item types: lowercase letter, space, name, newline
    while (1) {
        
        int tok = fgetc(istr);

        if (tok == '\n') {
            break;
        }
        
        assert(isalpha(tok) && islower(tok));
        assert(fgetc(istr) == ' ');
            
        int item_type = desc->num_item_types++;
        assert(item_type < MAX_ITEM_TYPES);

        item_info_t* info = desc->item_info + item_type;

        info->abbrev = tok;

        size_t len=0;

        while (1) {
            tok = fgetc(istr);
            if (tok == '\n') { break; }
            assert(isalpha(tok));
            info->name[len++] = toupper(tok);
            assert(len < MAX_ITEM_NAME);
        }

        assert(len > 0);
        info->name[len] = 0;

        printf("item type %d has abbrev %c and name '%s'\n",
               item_type, info->abbrev, info->name);

        item_lookup[info->abbrev-'a'] = item_type;

    }

    char* cgrid = (char*)malloc(MAX_BOARD_SIZE);

    size_t map_offset = 0;
    size_t cur_col = 0;

    ++desc->max_objects; // always 1 dummy object

    while (1) {

        assert(map_offset == desc->cols*desc->rows + cur_col);
        
        int tok = fgetc(istr);
        if (tok == -1) { break; }

        if (tok == '\n') {

            if (!desc->rows) {
                desc->cols = cur_col;
            }

            assert(cur_col == desc->cols);
            
            ++desc->rows;
            assert(desc->rows <= MAX_BOARD_ROWS);
            cur_col = 0;
            
            continue;
            
        }

        if (tok != '#' && tok != ' ') {
            ++desc->max_objects;
        }

        cgrid[map_offset++] = tok;
        cur_col += 1;
        assert(cur_col <= MAX_BOARD_COLS);

    }

    desc->size = desc->rows * desc->cols;

    printf("board is %d rows x %d cols with %d max objects and %d item types\n",
           (int)desc->rows, (int)desc->cols,
           (int)desc->max_objects, (int)desc->num_item_types);
           

    map_offset = 0;
    size_t object_idx = 1;

    object_t* objects = game_objects_alloc(desc->max_objects);

    for (size_t i=0; i<desc->rows; ++i) {
        
        for (size_t j=0; j<desc->cols; ++j, ++map_offset) {
            
            char c = cgrid[map_offset];

            if (i == 0 || i+1 == desc->rows || j == 0 || j+1 == desc->cols) {
                assert(c == '#');
                continue;
            } else if (c == ' ') {
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
                    object->type = OBJECT_ITEM_TEXT;
                } else {
                    object->type = OBJECT_ITEM;
                }

                object->subtype = item_type;
                
            } else {

                int attr_type = lookup_attr_type(c);
                
                if (attr_type != -1) {
                    
                    object->type = OBJECT_ATTR_TEXT;
                    object->subtype = attr_type;
                    
                } else {

                    int word_type = lookup_word_type(c);
                    
                    if (word_type != -1) {

                        object->type = OBJECT_WORD_TEXT;
                        object->subtype = word_type;

                    } else {

                        assert( 0 && "unexpected character in input!" );

                    }

                }
                        
            }

        }
            
        
    }

    free(cgrid);

    *pdesc = desc;
    *pobjects = objects;
    
}

// assert no overlapping objects, everything facing right
void game_print(const game_state_t* state) {

    const game_desc_t* desc = state->desc;

    size_t map_offset = 0;

    for (size_t i=0; i<desc->rows; ++i) {
        for (size_t j=0; j<desc->cols; ++j, ++map_offset) {

            object_handle_t oidx = state->cells[map_offset];

            if (i == 0 || j == 0 || i == desc->rows - 1 || j == desc->cols - 1) {
                
                assert(oidx == 0);
                printf("#");
                
            } else if (!oidx) {
                
                printf(" ");
                
            } else {
                
                const object_t* object = state->objects + oidx;
                assert(object->type != OBJECT_NONE);

                char ochar = ' ';

                if (object->type == OBJECT_ITEM) {
                    ochar = desc->item_info[object->subtype].abbrev;
                } else if (object->type == OBJECT_ITEM_TEXT) {
                    ochar = toupper(desc->item_info[object->subtype].abbrev);
                } else if (object->type == OBJECT_ATTR_TEXT) {
                    ochar = ATTR_TYPE_CHARS[object->subtype];
                } else {
                    assert(object->type == OBJECT_WORD_TEXT);
                    ochar = WORD_TYPE_CHARS[object->subtype];
                }
                
                printf("%c", ochar);
                
            }
            
        }

        printf("\n");
        
    }
    
}
