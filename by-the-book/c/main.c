#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "vm.h"
#include "test/unit.h"

const char* VERSION = "v0.0.1";
bool DEBUG_TRACE = false;

char* readFile(const char* fname) {
    /* declare a file pointer */
    FILE    *infile;
    char    *buffer;
    size_t    numbytes;
    /* open an existing file for reading */
    infile = fopen(fname, "r");
    /* quit if the file does not exist */
    if(infile == NULL) {
        printf("Error: not enough file\n");
        exit(1);
    }
    /* Get the number of bytes */
    fseek(infile, 0L, SEEK_END);
    numbytes = ftell(infile);
    if (numbytes > 1<<16) {
        printf("Error: too much file\n");
        exit(1);
    }
    /* reset the file position indicator to 
    the beginning of the file */
    fseek(infile, 0L, SEEK_SET);	
    /* grab sufficient memory for the 
    buffer to hold the text */
    buffer = (char*)calloc(numbytes, sizeof(char));	
    /* copy all the text into the buffer */
    fread(buffer, sizeof(char), numbytes, infile);
    fclose(infile);
    return buffer;
}

int main(int argc, char** argv) {
#define EQ(a, b) (strncmp(a, b, 1024) == 0)
    bool test = true;
    for (int i = 1; i < argc; i++) {
        if (EQ(argv[i], "--debug") || EQ(argv[i], "-d")) {
            DEBUG_TRACE = true;
            printf("====== DEBUG_TRACE=true\n");
        } else if (EQ(argv[i], "--tests")) {
            test = true;
        } else if (EQ(argv[i], "--help") || EQ(argv[i], "-h")) {
            printf("roguh's Lox C VM (2025) version %s\n"
                   "Usage: %s [--debug] [--command|-c string] [--tests] [FILES...]\n"
                   "\n"
                   "-c COMMAND or --command COMMAND\n"
                   "    Evaluate the given COMMAND.\n"
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
            interpretString(argv[i + 1]);
            i++;
            test = false;
        } else {
            char* contents = readFile(argv[i]);
            interpretString(contents);
            free(contents);
            test = false;
        }
    }
    if (test) {
        testAll();
    }
    return 0;
}
