#include <stdio.h>
#include <stdlib.h>

#include "src/common.h"
#include "src/scanner.h"
#include "src/vm.h"
#include "src/test/unit.h"

const char* VERSION = "v0.0.1";
bool DEBUG_TRACE = false;

static void repl(void) {
    int chunk = 0;
    Chunk chunks[1024] = {0};
    char line[1<<20] = {0};
    initVM();
    initChunk(&chunks[0]);
    while (true) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");;
            break;
        }
        interpretStream(line, &chunks[chunk]);
    }
    chunk = 1;
    for (int i = 0; i < chunk; i++) {
        freeChunk(&chunks[i]);
    }
    freeVM();
}

static char* readFile(const char* fname) {
    /* declare a file pointer */
    FILE    *infile;
    char    *buffer;
    size_t    numbytes;
    /* open an existing file for reading */
    infile = fopen(fname, "r");
    /* quit if the file does not exist */
    if(infile == NULL) {
        ERR_PRINT("Error: not enough file\n");
        exit(1);
    }
    /* Get the number of bytes */
    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);
    if (numbytes > 1<<24) {
        ERR_PRINT("Error: too much file\n");
        exit(1);
    }
    /* reset the file position indicator to
    the beginning of the file */
    fseek(infile, 0L, SEEK_SET);	
    /* grab sufficient memory for the
    buffer to hold the text */
    buffer = (char*)calloc(numbytes + 1, sizeof(char));	
    /* copy all the text into the buffer */
    fread(buffer, sizeof(char), numbytes, infile);
    buffer[numbytes] = '\0';
    fclose(infile);
    return buffer;
}

int main(int argc, char** argv) {
#define EQ(a, b) (strncmp(a, b, 1024) == 0)
    bool test = false;
    bool ran = false;
    for (int i = 1; i < argc; i++) {
        if (EQ(argv[i], "--debug") || EQ(argv[i], "-d")) {
            DEBUG_TRACE = true;
            ERR_PRINT("====== DEBUG_TRACE=true\n");
        } else if (EQ(argv[i], "--tests")) {
            test = true;
            ran = true;
        } else if (EQ(argv[i], "--help") || EQ(argv[i], "-h")) {
            ERR_PRINT("roguh's Lox C VM (2025) version %s\n"
                   "Usage: %s [--debug] [--command|-c string] [--tests] [FILES...]\n"
                   "\n"
                   "(no arguments)\n"
                   "    Start a REPL.\n"
                   "-c COMMAND or --command COMMAND\n"
                   "    Evaluate the given COMMAND.\n"
                   "-l CODE or --lex CODE\n"
                   "    Tokenify (lex) the given CODE.\n"
                   "-d CODE or --dis CODE\n"
                   "    Compile and print the given CODE as c-lox bytecode.\n"
                   "--debug\n"
                   "    Enable debug-level tracing commands.\n"
                   "--tests\n"
                   "    Run internal language tests.\n"
                   "FILES\n"
                   "    Runs each file and -c/--code command in order.\n"
                   "-h or --help\n"
                   "    Print this message\n"
                   "-V or --version\n"
                   "    Print the program version %s\n"
                   "\n"
                   "\n"
                   "Happy coding!\n",
                   VERSION, argv[0], VERSION
            );
            return 0;
        } else if (EQ(argv[i], "-c") || EQ(argv[i], "--command")) {
            interpret(argv[i + 1]);
            i++;
            ran = true;
        } else if (EQ(argv[i], "-x") || EQ(argv[i], "--lex")) {
            scanAndPrint(argv[i + 1]);
            i++;
            ran = true;
        } else if (EQ(argv[i], "-d") || EQ(argv[i], "--dis")) {
            compileAndPrint(argv[i + 1]);
            i++;
            ran = true;
        } else {
            char* contents = readFile(argv[i]);
            interpret(contents);
            free(contents);
            ran = true;
        }
    }
    if (test) {
        testAll();
    }
    if (!ran) {
        repl();
    }
    return 0;
}
