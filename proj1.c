#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#include "proj1.h"
#include "macro.h"

#define GETLINE_MULTIPLIER (2)

typedef enum {
    state_plaintext,
    state_escape,
    state_macro,
    state_arg,
} state_t;

Macro *defined;
int numDefined;
int sizeDefined;

state_t state;

StringBuf parseHelper(StringBuf input, StringBuf output);
void removeComment(FILE *fp, StringBuf input);
StringBuf fullExpand(StringBuf input);
void parseFunc(StringBuf input, StringBuf output, StringBuf alreadyRead, StringBuf *args, bool base);

bool
checkDef(char *name) {
    for(int i = 0; i < numDefined; i++) {
        if(strcmp(defined[i]->name->information, name) == 0) {
            return true;
        }
    }
    return false;
}

StringBuf def(StringBuf name, StringBuf value, StringBuf output) {
    StringBuf answer = stringBufCreate();

    if (!checkDef(name->information)) {
        if(numDefined >= sizeDefined) {
            sizeDefined *= GETLINE_MULTIPLIER;
            defined = realloc(defined, sizeDefined * sizeof(struct macro));

            assert(defined);
        }

        Macro new_def = macroCreate(name, value);
        defined[numDefined] = new_def;
        numDefined++;
    }

    else {
        DIE("[ERROR] %s", "Macro already defined");
    }

    return answer;
}

StringBuf undef(StringBuf name, StringBuf output) {
    StringBuf answer = stringBufCreate();

    if (checkDef(name->information)) {
        for(int i = 0; i < numDefined; i++) {
            if(strcmp(defined[i]->name->information, name->information) == 0) {
                defined[i] = NULL;
                break;
            }
        }
    }
    else {
        DIE("[ERROR] %s", "Undefining nonexistent macro");
    }

    return answer;
}

StringBuf if_macro(StringBuf cond, StringBuf thenArg, StringBuf elseArg, StringBuf output) {
    StringBuf newAlreadyRead = stringBufCreate();

    if (cond->information != NULL) {
        newAlreadyRead = parseHelper(thenArg, output);
    }
    else {
        newAlreadyRead = parseHelper(elseArg, output);
    }

    return newAlreadyRead;
}

StringBuf ifdef(StringBuf cond, StringBuf thenArg, StringBuf elseArg, StringBuf output) {
    StringBuf newAlreadyRead = stringBufCreate();

    if (checkDef(cond->information)) {
        newAlreadyRead = parseHelper(thenArg, output);
    }
    else {
        newAlreadyRead = parseHelper(elseArg, output);
    }

    return newAlreadyRead;
}

StringBuf include(StringBuf name, StringBuf output) {
    FILE *fp = fopen(name->information, "r");
    StringBuf input = stringBufCreate();
    removeComment(fp, input);
    fclose(fp);

    StringBuf newAlreadyRead = parseHelper(input, output);
    stringBufDestroy(input);

    return newAlreadyRead;
}

StringBuf expandafter(StringBuf before, StringBuf after, StringBuf output) {
    StringBuf clean = fullExpand(after);
    string_putstring(before, clean);
    StringBuf combined = before;

    StringBuf newAlreadyRead = parseHelper(combined, output);
    stringBufDestroy(combined);

    return newAlreadyRead;
}

StringBuf expandMacro(char *macroName, StringBuf arg, StringBuf output) {
    StringBuf fullValue = stringBufCreate();
    StringBuf value = stringBufCreate();

    for(int i = 0; i < numDefined; i++) {
        if(strcmp(defined[i]->name->information, macroName) == 0) {
            string_putstring(value, defined[i]->value);
            break;
        }
    }

    for (int i = 0; i < value->n; i++) {
        char c = value->information[i];
        if((c == '#') && (value->information[i-1] != '\\')) {
            string_putstring(fullValue, value);
        }
        else {
            string_putchar(fullValue, c);
        }
    }

    StringBuf newAlreadyRead = parseHelper(fullValue, output);
    stringBufDestroy(fullValue);

    return newAlreadyRead;
}

StringBuf
processMacro(StringBuf output, char *macroName, StringBuf *args) {
    StringBuf expandedMacro = stringBufCreate();

    if (strcmp(macroName, "def") == 0) {
        string_putstring(expandedMacro, def(args[0], args[1], output));
    }
    else if (strcmp(macroName, "undef") == 0) {
        string_putstring(expandedMacro, undef(args[0], output));
    }
    else if (strcmp(macroName, "if") == 0) {
        string_putstring(expandedMacro, if_macro(args[0], args[1], args[2], output));
    }
    else if (strcmp(macroName, "ifdef") == 0) {
        string_putstring(expandedMacro, if_macro(args[0], args[1], args[2], output));
    }
    else if (strcmp(macroName, "include") == 0) {
        string_putstring(expandedMacro, include(args[0], output));
    }    
    else if (strcmp(macroName, "expandafter") == 0) {
        string_putstring(expandedMacro, expandafter(args[0], args[1], output));
    }

    if (checkDef(macroName)) {
        string_putstring(expandedMacro, expandMacro(macroName, args[0], output));
    }
    else {
        DIE("[ERROR] %s", "Macro name not defined");
    }

    return expandedMacro;
}

int
numArgs(char *macroName) {
    int numArg;

    if (strcmp(macroName, "def") == 0) {
        numArg = 2;
    }
    else if (strcmp(macroName, "undef") == 0) {
        numArg = 1;
    }
    else if (strcmp(macroName, "if") == 0) {
        numArg = 3;
    }
    else if (strcmp(macroName, "ifdef") == 0) {
        numArg = 3;
    }
    else if (strcmp(macroName, "include") == 0) {
        numArg = 1;
    }    
    else if (strcmp(macroName, "expandafter") == 0) {
        numArg = 2;
    }

    if (checkDef(macroName)) {
        numArg = 1;
    }
    else {
        DIE("[ERROR] %s", "Macro name not defined");
    }

    return numArg;
}

void
parseFunc(StringBuf input, StringBuf output, StringBuf alreadyRead, StringBuf *args, bool base) {
    int c = 0; // where we currently are in input string
    int numArg; // number of arguments expected
    int numArgFound; // number of arguments in args
    int bc; // brace counter
    printf("Number of characters in input %ld", input->n);
    printf("HELLO");
    while(c < input->n) {
    
        char ch = input->information[c];
        switch(state) {
            case state_plaintext:
                if (ch == '%') {
                    printf("left a comment");
                }
                else if (ch == '\\') {
                    printf("%c%d", ch, c);
                    printf("switching to escape");
                    state = state_escape;
                }
                else {
                    string_putchar(output, ch);
                }
                break;

            case state_escape:
                // previous character was an escape character
                if (isalnum(ch)) {
                    printf("%c%d", ch, c);
                    printf("switching to macro");
                    state = state_macro;
                    string_putchar(alreadyRead, ch);
                }
                else {
                    printf("switching to plaintext");
                    state = state_plaintext;
                    string_putchar(output, ch);
                }
                break;

            case state_macro:
                // in state of reading macro name
                if (isalnum(ch)) {
                    string_putchar(alreadyRead, ch);
                }
                else if (c == '{') {
                    printf("switching to arg");
                    state = state_arg;
                    numArg = numArgs(alreadyRead->information);
                    numArgFound = 0;
                    bc = 1;
                }
                else {
                    DIE("[ERROR] %s", "Invalid macro name");
                }

                break;

            case state_arg:
                // found a macro, reading in arguments

                if (ch == '}') {
                    bc--;
                    if (bc == 0) {
                        numArgFound++;
                        if (numArgFound == numArg) {
                            StringBuf newAlreadyRead = processMacro(output, alreadyRead->information, args);
                            
                            // no fragment to process
                            if (newAlreadyRead->information == NULL) {
                                printf("switching to plaintext after processing");
                                state = state_plaintext;
                                stringBufDestroy(alreadyRead);
                                alreadyRead = stringBufCreate();
                                for (int i = 0; i < 3; i++) {
                                    stringBufDestroy(args[i]);
                                    args[i] = stringBufCreate();
                                }
                            }
                            // macro fragment in alreadyRead, keep going
                            else {
                                printf("switching to macro after processing");
                                state = state_macro;
                            }
                        }
                    }
                    else {
                        string_putchar(args[numArgFound], ch);
                    }
                }
                else if (ch == '{') {
                    bc++;
                    if (bc > 1) {
                        string_putchar(args[numArgFound], ch);
                    }
                }
                else {
                    if (base == true) {
                        if ((bc == 0) && (numArgFound > 0) && (ch != '{')) {
                            // expecting another argument but not getting it
                            DIE("[ERROR] %s", "Invalid arguments");
                        }
                        if (c == input->n-1) {
                            DIE("[ERROR] %s", "Too few arguments");
                        }
                        string_putchar(args[numArgFound], ch);
                    }

                    else {
                        return;
                    }      
                }

                break; 
        }

        c++;

    }
}

void
removeComment(FILE *fp, StringBuf input) {
    int b;
    int c = fgetc(fp);

    while(c != EOF) {
        b = c;
        c = fgetc(fp);

        if ((c == '%') && (b != '\\')) {
            c = fgetc(fp);

            while ((c != '\n') && (c!= EOF)) {
                b = c;
                c = fgetc(fp);
            }

            while (isblank(c)) {
                b = c;
                c = fgetc(fp);
            }
        }

        string_putchar(input, c);
    }
}

StringBuf
outputBuffer(StringBuf output) {
    StringBuf cleanOutput = stringBufCreate();

    int i = 0;
    while(i < output->n) {
        char c = output->information[i];
        if (c == '\\') {
            char d = output->information[i+1];
            if ((d == '\\') | (d == '#') | (d == '%') | (d == '{') | (d == '}')) {
                string_putchar(cleanOutput, d);
                i++;
            }
        }
        else {
            string_putchar(cleanOutput, c);
        }
        i++;
    }

    return cleanOutput;
}

StringBuf
fullExpand(StringBuf input) {
    StringBuf output = stringBufCreate();
    state = state_plaintext;
    StringBuf alreadyRead = stringBufCreate();
    StringBuf *args = malloc(sizeof(struct stringBuf) * 3);
    for(int i = 0; i < 3; i++) {
        args[i] = stringBufCreate();
    }

    parseFunc(input, output, alreadyRead, args, true);
    StringBuf final = outputBuffer(output);
    stringBufDestroy(input);
    stringBufDestroy(output);
    stringBufDestroy(alreadyRead);
    for(int i = 0; i < 3; i++) {
        stringBufDestroy(args[i]);
    }
    free(args);

    return final;
}

StringBuf
parseHelper(StringBuf input, StringBuf output) {
    StringBuf alreadyRead = stringBufCreate();
    StringBuf *args = malloc(sizeof(struct stringBuf) * 3);
    for(int i = 0; i < 3; i++) {
        args[i] = stringBufCreate();
    }

    parseFunc(input, output, alreadyRead, args, false);
    for(int i = 0; i < 3; i++) {
        stringBufDestroy(args[i]);
    }
    free(args);

    return alreadyRead;
}

int
main(int argc, char **argv) {
    // initialize global array of defined macros
    sizeDefined = 10;
    defined = malloc(sizeof(struct macro) * sizeDefined);
    for(int i = 0; i < 10; i++) {
        defined[i] = NULL;
    }
    numDefined = 0;

    // initialize file input
    StringBuf input = stringBufCreate();

    // put contents of all files/stdin into StringBuf input
    if (argc < 1) {
        FILE *fp = stdin;
        if (fp == NULL) {
            DIE("[ERROR] %s", "Null pointer");
            //fprintf(stderr, "[ERROR]: Null pointer");
        } 
        removeComment(fp, input);
    }
    else {
        for(int i = 0; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (fp == NULL) {
                DIE("[ERROR] %s", "Null pointer");
            } 
            removeComment(fp, input);
        }
    }

    StringBuf final = fullExpand(input);
    for (int i = 0; i < final->n; i++) {
        putchar(final->information[i]);
    }

    stringBufDestroy(final);

    for(int i = 0; i < numDefined; i++) {
        macroDestroy(defined[i]);
    }
    free(defined);

    return 0;
}
