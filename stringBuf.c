#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "stringBuf.h"

#define GETLINE_INITIAL_SIZE (8)
#define GETLINE_MULTIPLIER (2)

StringBuf stringBufCreate() {
    StringBuf s = malloc(sizeof(struct stringBuf));
    assert(s);

    s->capacity = GETLINE_INITIAL_SIZE;
    s->n = 0;
    s->information = calloc(s->capacity+1, sizeof(char));
    assert(s->information);

    s->information[s->capacity] = '\0';

    return s;
}

void stringBufDestroy(StringBuf s) {
    free(s->information);
    free(s);
}

void string_putchar(StringBuf s, char c) {
    if(s->n+1 >= s->capacity) {
        s->capacity *= GETLINE_MULTIPLIER;
        s->information = realloc(s->information, s->capacity * sizeof(char));

        assert(s->information);
    }

    s->information[s->n++] = c;
    s->information[s->n] = '\0';
}

void string_putstring(StringBuf s, StringBuf other) {
    while(s->n+1+other->n >= s->capacity) {
        s->capacity *= GETLINE_MULTIPLIER;
        s->information = realloc(s->information, s->capacity * sizeof(char));

        assert(s->information);
    }

    int c = 0;
    int i;
    for(i = s->n; i < s->n+other->n; i++) {
        s->information[i] = other->information[c];
        c++;
    }
    s->information[i] = '\0';

    stringBufDestroy(other);
}
