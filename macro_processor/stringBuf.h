// interface for string-like data structure

struct stringBuf
{
    char *information;
    size_t n;
    size_t capacity;
};

typedef struct stringBuf *StringBuf;

// create string buffer
StringBuf stringBufCreate();

// free all space used by string buffer
void stringBufDestroy(StringBuf s);

// append a character to string
void string_putchar(StringBuf s, char c);

void string_putstring(StringBuf s, StringBuf other);