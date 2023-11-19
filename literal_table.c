#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "literal_table.h"
#include "utilities.h"

// constant table entries
typedef struct literal_table_node {
    struct literal_table_node *next;
    const char *text;
    word_type value;
    unsigned int offset;
} literal_table_node;

static literal_table_node *first;
static literal_table_node *last;
static unsigned int size;

static literal_table_node *iteration_next;

// Return the size (in words/entries) in the literal table
extern unsigned int literal_table_size()
{
    return size;
}

// is the literal_table empty?
extern bool literal_table_empty()
{
    return first == NULL;
}

// is the literal_table full?
extern bool literal_table_full()
{
    return false;
}

// initialize the literal_table
extern void literal_table_initialize()
{
    first = NULL;
    last = NULL;
    size = 0;
    iteration_next = NULL;
}

// Return the offset of sought if it is in the table,
// otherwise return -1.
extern int literal_table_find_offset(const char *sought, word_type value)
{
    literal_table_node *i_node = first;
    while (i_node != NULL)
    {
        if (value == i_node->value)
            return i_node->offset;
        i_node = i_node->next;
    }
    return -1;
}

// Return true just when sought is in the table
extern bool literal_table_present(const char *sought, word_type value)
{
    return literal_table_find_offset(sought, value) != -1;
}

// Return the word offset for val_string/value
// entering it in the table if it's not already present
extern unsigned int literal_table_lookup(const char *val_string, word_type value)
{
    int offset = literal_table_find_offset(val_string, value);
    if (offset != -1)
        return offset;
    // Enter new value in table
    literal_table_node *new_node = (literal_table_node *)malloc(sizeof(literal_table_node));
    new_node->text = val_string;
    new_node->value = value;
    new_node->next = NULL;
    offset = size;
    new_node->offset = size++;

    if (first == NULL)
    {
        first = new_node;
        last = new_node;
    }
    else 
    {
        last->next = new_node;
        last = new_node;
    }
    return offset;
}

// === iteration helpers ===

// Start an iteration over the literal table
// which can extract the elements
extern void literal_table_start_iteration()
{
    if (iteration_next != NULL)
        bail_with_error("Cannot start iteration in literal_table. Current iteration already running.");
    iteration_next = first;
}

// End the current iteration over the literal table.
extern void literal_table_end_iteration()
{
    iteration_next = NULL;
}

// Is there another literal in the literal table?
extern bool literal_table_iteration_has_next()
{
    return iteration_next != NULL;
}

// Return the next word_type in the literal table
// and advance the iteration
extern word_type literal_table_iteration_next()
{
    if (iteration_next == NULL)
        bail_with_error("Next iteration not found.");
    word_type value = iteration_next->value;
    iteration_next = iteration_next->next;
    return value;
}