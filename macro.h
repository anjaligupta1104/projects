// interface for built-in macros
#include "stringBuf.h"

struct macro {
    StringBuf name;
    StringBuf value;
};

typedef struct macro *Macro;

// create string buffer with name and replacement value
Macro macroCreate(StringBuf name, StringBuf value);

// free all space used by string buffer
void macroDestroy(Macro m);