#include <stdio.h>
#include "array.h"


void
init_array(Array *a, size_t initial_size)
{
    a->array = malloc(initial_size * sizeof(T));
    a->used = 0;
    a->size = initial_size;
}

void
push_array(Array *a, T element)
{
    if (a->used == a->size) {
        // below could potentially be "+= sizeof(T)" for space efficiency 
        // !! this causes a crash with "invalid next location"
        // aka the heap has been smashed.
        // TODO: Experiment with lower values to expand array size by.
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(T));
    }
    a->array[a->used++] = element;
}

void
print_array(Array *a)
{
    size_t i;

    printf("\n");
    printf("Size of array: %lu\n", a->size);
    printf("Number of members of array: %d", (int)a->used);
    printf("\nMembers of window list: \n");

    if (a->used != 0)
        for (i = 0; i < a->used; ++i) {
            printf("%lu\n", a->array[i]);
        }
    printf("\n");
}

void
insert_array(Array *a, T element, int position)
{
    int i;
    a->array = realloc(a->array, a->size * sizeof(T) + sizeof(T));
    a->size += 1;
    a->used += 1;

    for (i = a->used; i >= position; --i) {
        a->array[i + 1] = a->array[i];
    }

    a->array[position] = element;
}

int
find_array(Array *a, T element)
    {
        int i = 0;

        while ((size_t)i < (a->used) && a->array[i] != element) ++i;

        return (size_t)i == a->used ? -1 : i;
    }


void
remove_array(Array *a, T element)
{
    int i;
    int position;
    if ((position = find_array(a, element)) != -1) {
        for (i = position; (size_t)i < a->used; ++i) {
            a->array[i] = a->array[i + 1];
        }
        a->array[a->used] = 0;

        a->used--;
        a->array = realloc(a->array, a->size * sizeof(T));
    }
}

void
move_array(Array *a, T element, int new_position)
{
    remove_array(a, element);
    insert_array(a, element, new_position);
}


void 
free_array(Array *a)
{
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}
