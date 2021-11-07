#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "macro.h"

Macro macroCreate(StringBuf name, StringBuf value) {
    Macro m = malloc(sizeof(struct macro));
    assert(m);

    m->name = name;
    m->value = value;

    return m;
}

void macroDestroy(Macro m) {
    stringBufDestroy(m->name);
    stringBufDestroy(m->value);
    free(m);
}