/*

Project 0: Josh Lewis
Date: 1/28/2018

Program: Count instances of words entered through command line.

Ex:
> ./main cat nap dog
Given input:
 cat
 .
Expected output:
 Looking for 3 words
 Result:
 cat:1
 nap:0
 dog:0
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smp0_tests.h"

#define LENGTH(s) (sizeof(s) / sizeof(*s))

/* Structures */
typedef struct {
  char *word;
  int counter;
} WordCountEntry;

int process_stream (WordCountEntry entries[], int entry_count)
{
    short line_count = 0;
    char *token;
    char *buffer;
    
    // Dynamically allocate memory for buffer
    buffer = (char *)malloc(sizeof(WordCountEntry) * (entry_count - 1));

    // Get size of buffer
    int buffer_size = sizeof(buffer)/sizeof(buffer[0]);
    
    while (fgets(buffer, buffer_size, stdin) != NULL) {
        if (!strcmp(buffer, ".\n")) {
            break;
        }
        
       // Create token to include space, tab, and newline delimeters
       token = strtok(buffer, " \t\n");
        
        while (token != NULL) {
            for (int i = 0; i < entry_count; i++) {
                if (!strcmp(entries[i].word, token)) {
                    entries[i].counter++;
                }
            }
            token = strtok(NULL, " \t\n");
        }
        line_count++;
    }
    return line_count;
}

void print_result (WordCountEntry entries[], int entry_count, FILE *outputFile)
{

    // Prints to standard out if no output file is open
    fprintf (stdout, "\nResult:\n");
    for (int i = 0; i < entry_count; i++) {
        fprintf (stdout, "%s:%d\n", entries[i].word, entries[i].counter);
    }

    // Checks for existence of output file
    if(outputFile != NULL) {
        fprintf (outputFile, "Result:\n");
        for (int i = 0; i < entry_count; i++) {
            fprintf (outputFile, "%s:%d\n", entries[i].word, entries[i].counter);
        }
        
        // Close file if necessary
        fclose(outputFile);
    }
}

void printHelp(const char *name)
{
  fprintf(stderr, "usage: %s [-h] <word1> ... <wordN>\n", name);
  fprintf(stderr, "usage: type [-f]FILENAME to create an output file with results.\n");
}


int main(int argc, char **argv)
{
    const char *prog_name = *argv;
    char *filename;
    FILE* outputFile;
    
    WordCountEntry *entries;
    int entry_count = 0;
    
    // Allocate memory for WordCountEntry
    entries = malloc(sizeof(WordCountEntry)*(argc-1));
    
    if((entries = malloc(sizeof(WordCountEntry)*(argc-1))) == NULL) {
        fprintf(stderr, "Cannot allocate appropriate memory.");
    }
    
    /* Entry point for the testrunner program */
    if (argc > 1 && !strcmp(argv[1], "-test")) {
    run_smp0_tests(argc - 1, argv + 1);
    return EXIT_SUCCESS;
    }

    // Increment by 1 to skip program name
    argv++;
    
    // Switch statement menu for entries that begin with "-"
    while (*argv != NULL) {
        if ((*argv)[0] == '-') {
            switch ((*argv)[1]) {
                case 'h':
                    printHelp (prog_name);
                    break;
                case 'f':
                    filename = *argv+2;
                    fprintf(stdout, "Result will be printed to %s.\n", filename);
                    if((outputFile = fopen(filename, "w")) == NULL) { // check if file open failed
                        fprintf(stderr, "Can't open output file %s!\n", filename);
                        free(entries); // free up memory
                        return EXIT_FAILURE;
                    }
                    break;
                default:
                    fprintf (stderr, "%s: Invalid option %s. Use -h for help.\n",
                    prog_name, *argv);
            }
        } else {
            // Execute if "-" was not entered after program name
            entries[entry_count].word = *argv;
            entries[entry_count].counter = 0;
            entry_count++;
         }
    argv++;
  }
    
  if (entry_count == 0) {
    fprintf(stderr, "%s: Please supply at least one word. Use -h for help.\n",
    prog_name);
    return EXIT_FAILURE;
  }
  if (entry_count == 1) {
      fprintf(stdout, "Looking for a single word\n");
  }
  else {
      fprintf(stdout, "Looking for %d words\n", entry_count);
  }

    process_stream(entries, entry_count);
    
    if((outputFile = fopen(filename, "w")) != NULL) {
        fprintf(stderr, "The result is saved to %s.\n", filename);
    }
    
    print_result(entries, entry_count, outputFile);

  return EXIT_SUCCESS;
}
