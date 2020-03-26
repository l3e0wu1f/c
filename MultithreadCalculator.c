/*
 * Name: Josh Lewis
 * Email: jblewis@mail.fhsu.edu
 * Project 3: calc.c - Multithreaded calculator
 * Date: 4/5/2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <ctype.h>
#include <pthread.h>
#include "calc.h"

#define BUF_SIZE 500

pthread_t adderThread;
pthread_t degrouperThread;
pthread_t multiplierThread;
pthread_t readerThread;
pthread_t sentinelThread;

char buffer[BUF_SIZE];
int num_ops;

static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

// add condition flag variables
struct progress_t {
    int add;
    int mult;
    int group;
} progress;

static sem_t progress_lock; // use a semaphore

/* Utiltity functions provided for your convenience */

/* int2string converts an integer into a string and writes it in the
   passed char array s, which should be of reasonable size (e.g., 20
   characters).  */
char *int2string(int i, char *s)
{
    sprintf(s, "%d", i);
    return s;
}

/* string2int just calls atoi() */
int string2int(const char *s)
{
    return atoi(s);
}

/* isNumeric just calls isdigit() */
int isNumeric(char c)
{
    return isdigit(c);
}

/* End utility functions */


void printErrorAndExit(char *msg)
{
    msg = msg ? msg : "An unspecified error occured!";
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int timeToFinish()
{
    /* be careful: timeToFinish() also accesses buffer */
    return buffer[0] == '.';
}

/* Looks for an addition symbol "+" surrounded by two numbers, e.g. "5+6"
   and, if found, adds the two numbers and replaces the addition subexpression 
   with the result ("(5+6)*8" becomes "(11)*8")--remember, you don't have
   to worry about associativity! */
void *adder(void *arg)
{
    int bufferlen;
    int value1, value2;
    int startOffset, remainderOffset;
    int i;
    int result;
    int changed; // indcates buffer status
    char nString[50];

    while (1) {
        // lock buffer
        pthread_mutex_lock(&buffer_lock);
        
        if (timeToFinish()) {
            pthread_mutex_unlock(&buffer_lock);
            return NULL;
        }

        /* storing this prevents having to recalculate it in the loop */
        bufferlen = strlen(buffer);
        changed = 0;

        for (i = 0; i < bufferlen; i++) {
            if(buffer[i] == ';') { break; }
            if(isdigit(buffer[i])) {
                startOffset = i;
                value1 = atoi(buffer + i);
                while(isdigit(buffer[i])) {
                    i++;
                }
                if(buffer[i] != '+' || !isdigit(buffer[i+1])) { continue; }
                
                value2 = atoi(buffer + i + 1);
                result = value1 + value2;
                
                do {
                    i++;
                } while(isdigit(buffer[i]));
                
                remainderOffset = i;
                
                // Convert result to string
                sprintf(nString, "%d", result);
                
                strcpy(buffer + startOffset, nString); // rearrange buffer
                strcpy(buffer + startOffset, nString); // rearrange buffer
                strcpy(buffer + startOffset + strlen(nString), buffer + remainderOffset);
                
                bufferlen = strlen(buffer);
                i = startOffset + strlen(nString) - 1;
                
                // indicate add has updated the buffer
                changed = 1;
                num_ops++;
            }
        }

	// Free lock
    pthread_mutex_unlock(&buffer_lock);
    sem_wait(&progress_lock); // step 6: check progress
    progress.add = changed? 2 : 1;
    sem_post(&progress_lock);
    
    sched_yield(); // let other thread run
    }
}

/* Looks for a multiplication symbol "*" surrounded by two numbers, e.g.
   "5*6" and, if found, multiplies the two numbers and replaces the
   mulitplication subexpression with the result ("1+(5*6)+8" becomes
   "1+(30)+8"). */
void *multiplier(void *arg)
{
    int bufferlen;
    int value1, value2;
    int startOffset, remainderOffset;
    int i;
    int result;
    int changed;
    char nString[50];
    
    while (1) {
        // lock buffer
        pthread_mutex_lock(&buffer_lock);
        
        if (timeToFinish()) {
            pthread_mutex_unlock(&buffer_lock);
            return NULL;
        }
        
        /* storing this prevents having to recalculate it in the loop */
        bufferlen = strlen(buffer);
        changed = 0;
        
        for (i = 0; i < bufferlen; i++) {
            if(buffer[i] == ';') { break; }
            if(isdigit(buffer[i])) {
                startOffset = i;
                value1 = atoi(buffer + i);
                while(isdigit(buffer[i])) {
                    i++;
                }
                if(buffer[i] != '*' || !isdigit(buffer[i+1])) { continue; }
                
                value2 = atoi(buffer + i + 1);
                result = value1 * value2;
                
                do {
                    i++;
                } while(isdigit(buffer[i]));
                
                remainderOffset = i;
                
                // Convert result to string
                sprintf(nString, "%d", result);
                
                strcpy(buffer + startOffset, nString); // rearrange buffer
                strcpy(buffer + startOffset, nString); // rearrange buffer
                strcpy(buffer + startOffset + strlen(nString), buffer + remainderOffset);
                
                bufferlen = strlen(buffer);
                i = startOffset + strlen(nString) - 1;
                changed = 1;
                num_ops++;
            }
        }
        
        // Free lock
        pthread_mutex_unlock(&buffer_lock);
        sem_wait(&progress_lock); // step 6: check progress
        progress.mult = changed? 2 : 1;
        sem_post(&progress_lock);
        
        sched_yield(); // let other thread run
    }
}


/* Looks for a number immediately surrounded by parentheses [e.g.
   "(56)"] in the buffer and, if found, removes the parentheses leaving
   only the surrounded number. */
void *degrouper(void *arg)
{
    int bufferlen;
    int startOffset;
    int i;
    int changed;

    while (1) {
        // lock buffer
        pthread_mutex_lock(&buffer_lock);
        
        if (timeToFinish()) {
            pthread_mutex_unlock(&buffer_lock);
            return NULL;
        }

	/* storing this prevents having to recalculate it in the loop */
	bufferlen = strlen(buffer);
    changed = 0;

	for (i = 0; i < bufferlen; i++) {
        if(buffer[i] == ';') { break; } // Let sentinel thread run
        
        if(buffer[i] == '(' && isdigit(buffer[i+1])) {
            startOffset = i;
            do {
                i++;
            } while(isdigit(buffer[i]));
            
            if(buffer[i] != ')') {
                i--;
                continue;
            }
            
            strcpy(buffer + i, buffer + i + 1);
            strcpy(buffer + startOffset, buffer + startOffset + 1);
            bufferlen -= 2;
            i = startOffset;
            
            changed = 1;
            num_ops++;
        }
	}

        // Free lock
        pthread_mutex_unlock(&buffer_lock);
        sem_wait(&progress_lock); // step 6: check progress
        progress.group = changed? 2 : 1;
        sem_post(&progress_lock);
        
        sched_yield(); // let other thread run
    }
}


/* sentinel waits for a number followed by a ; (e.g. "453;") to appear
   at the beginning of the buffer, indicating that the current
   expression has been fully reduced by the other threads and can now be
   output.  It then "dequeues" that expression (and trailing ;) so work can
   proceed on the next (if available). */
void *sentinel(void *arg)
{
    char numberBuffer[20];
    int bufferlen;
    int i;

    while (1) {
        // lock buffer
        pthread_mutex_lock(&buffer_lock);
        
        if (timeToFinish()) {
            pthread_mutex_unlock(&buffer_lock);
            return NULL;
        }

	/* storing this prevents having to recalculate it in the loop */
	bufferlen = strlen(buffer);
        
        for(i = 0; i < bufferlen; i++) {
            bufferlen = strlen(buffer);
        }

        for (i = 0; i < bufferlen; i++) {
            if (buffer[i] == ';') {
                if (i == 0) {
                    printErrorAndExit("Sentinel found empty expression!");
                } else {
                    /* null terminate the string */
                    numberBuffer[i] = '\0';
                    /* print out the number we've found */
                    printf("%s\n", numberBuffer);
                    /* shift the remainder of the string to the left */
                    strcpy(buffer, &buffer[i + 1]);
                    break;
                }
            } else if (!isdigit(buffer[i])) {
                break;
            } else {
                numberBuffer[i] = buffer[i];
            }
        }
        
        // step 6: check for progress
        if(buffer[0] != '\0') {
            sem_wait(&progress_lock);
            if(progress.add && progress.mult && progress.group) { // all fields set to 1 or 2
                if(progress.add > 1 || progress.mult > 1 || progress.group > 1) { // made progress, so reset to 0
                    progress.add = 0;
                    progress.mult = 0;
                    progress.group = 0;
                }
                else {
                    fprintf(stdout, "No progress can be made.\n");
                    exit(EXIT_FAILURE);
                }
            }
            sem_post(&progress_lock);
        }

        // Free lock
        pthread_mutex_unlock(&buffer_lock);
        sched_yield(); // let other thread run
    }
}

/* reader reads in lines of input from stdin and writes them to the
   buffer */
void *reader(void *arg)
{
    while (1) {
	char tBuffer[100];
	int currentlen;
	int newlen;
	int free;

	fgets(tBuffer, sizeof(tBuffer), stdin);

	newlen = strlen(tBuffer);

	/* if tBuffer comes back with a newline from fgets, remove it */
	if (tBuffer[newlen - 1] == '\n') {
	    /* shift null terminator left */
	    tBuffer[newlen - 1] = tBuffer[newlen];
	    newlen = strlen(tBuffer);
	}

        do { // lock buffer
            pthread_mutex_lock(&buffer_lock);
            currentlen = strlen(buffer);
            free = sizeof(buffer) - currentlen - 2;
            pthread_mutex_unlock(&buffer_lock);
            sched_yield();
        } while (free < newlen);

        pthread_mutex_lock(&buffer_lock);
        strcat(buffer, tBuffer);
        strcat(buffer, ";");
        
        sem_wait(&progress_lock);
        progress.add = 0;
        progress.mult = 0;
        progress.group = 0;
        sem_post(&progress_lock);
        
        pthread_mutex_unlock(&buffer_lock);

        /* Stop when user enters '.' */
        if (tBuffer[0] == '.') {
            return NULL;
        }
    }
}


/* Where it all begins */
int smp3_main(int argc, char **argv)
{
    void *arg = 0;		/* dummy value */

    /* let's create our threads */
    if (pthread_create(&multiplierThread, NULL, multiplier, arg)
	|| pthread_create(&adderThread, NULL, adder, arg)
	|| pthread_create(&degrouperThread, NULL, degrouper, arg)
	|| pthread_create(&sentinelThread, NULL, sentinel, arg)
	|| pthread_create(&readerThread, NULL, reader, arg)) {
	printErrorAndExit("Failed trying to create threads");
    }

    /* you need to join one of these threads... but which one? */
    pthread_detach(multiplierThread);
    pthread_detach(adderThread);
    pthread_detach(degrouperThread);
    pthread_detach(readerThread);
    pthread_join(sentinelThread, NULL); // main thread waits for this one to finish

    /* everything is finished, print out the number of operations performed */
    fprintf(stdout, "Performed a total of %d operations\n", num_ops);
    return EXIT_SUCCESS;
}
