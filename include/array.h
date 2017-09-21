#include <stdlib.h>
#define T unsigned long


typedef struct {
    T *array;
    size_t size;
    size_t used;
} Array;

void init_array(Array *a, size_t initial_size);
void push_array(Array *a, T element);
void insert_array(Array *a, T element, int position);
void remove_array(Array *a, T element);
void move_array(Array *a, T element, int new_position);
void print_array(Array *a);
void free_array(Array *a);
int find_array(Array *a, T element);
