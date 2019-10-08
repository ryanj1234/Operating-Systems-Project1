/**
 * included input files:
 *  asdf.txt
 *  fdsa.txt
 *  hjkl.txt
 *  hamlet.txt

 * to compile:
 *  gcc proj1.c
 *
 * to run with all input files:
 *  ./a.out 4 asdf.txt fdsa.txt hjkl.txt hamlet.txt results.txt
 * 
 * results will be written to results.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_CHAR    26

/**
 * print program usage to user if incorrect command line args are rec'd
 */
void print_usage(char fname[]) {
    printf("Usage:\n%s n file1.txt file2.txt ... filen.txt results.txt\n", fname);
}

/**
 * check if a given file exists. Will return 1 if file exists, otherwise 0
 */
int file_exists(char *fname) {
    struct stat buffer;

    int exists = stat(fname, &buffer);
    if(exists == 0)
        return 1;
    else
        return 0;
}

/**
 * check that all given files exist. Must provide the number of files in
 * num_files and an array of file names in file_names. Will return 0 if all
 * files are found, otherwise an error will be printed and -1 will be returned
 */
int check_files(int num_files, char *file_names[]) {
    for(int i = 0; i < num_files; i++) {
        if(!file_exists(file_names[i])) {
            printf("File %s does not exist!\n", file_names[i]);
            return -1;
        }
    }

    return 0;
}

/**
 * counts the number of times each letter of the alphabet is used in a given
 * file. Requires a file name and a buffer of at least size 26 to hold character
 * counts. 0 will be returned on success, otherwise -1 will be returned.
 */
int count_usage(char *fname, int *counts) {
    FILE *fp;
    char c;
    int idx;

    // try to open file
    fp = fopen(fname, "r");
    if(fp == NULL) {
        printf("Error opening file.\n");
        return -1;
    }

    // read characters until an end of file is reached
    while((c = fgetc(fp)) != EOF) {
        // ignore case of character by converting all to lower. The ascii value
        // is then converted to an index by subtracting the value of 'a' (97).
        // Thus, a->0, b->1, etc.
        idx = tolower(c) - 97;

        // verify index is within bounds. Will ignore any non A-Za-z chars
        if((idx >= 0) && (idx < NUM_CHAR)) {
            counts[idx]++;
        }
    }

    // close file
    fclose(fp);

    return 0;
}

int main(int argc, char *argv[]) {
    // ensure command line args are correct
    if(argc < 2) {
        fprintf(stderr, "Not enough command line arguments received\n");
        print_usage(argv[0]);
        return -1;
    }

    // convert number of files arg into an int
    int num_files = atoi(argv[1]);

    // abort if num_files is negative or zero
    if(num_files < 1) {
        fprintf(stderr, "Invalid number of files specified: %s\n", argv[1]);
        print_usage(argv[0]);
        return -2;
    }

    // check that the right amount of input files were received.
    if(num_files < (argc - 3)) {
        fprintf(stderr, "Too many file names received\n");
        return -2;
    }
    else if(num_files > (argc - 3)) {
        fprintf(stderr, "Not enough file names received\n");
        return -3;
    }

    // check that all files exist
    if(check_files(num_files, argv + 2) != 0) {
        printf("Execution ending early due to missing files...\n");
        return -4;
    }

    char *out_file = argv[argc - 1];

    // output file pointer
    FILE *of;

    // clear output file
    of = fopen(out_file, "w");
    fclose(of);

    // create thread mutex
    pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

    // counter for the number of threads
    int counter = 0;

    // create a thread for each file
    int pid;
    for(int i = 0; i < num_files; i++) {
        if(!(pid = fork())) {
            // lock mutex to increment thread count
            pthread_mutex_lock(&mutex1);
            counter++;
            pthread_mutex_unlock(&mutex1);

            // buffer to hold character counts
            int counts[NUM_CHAR];

            // zero out buffer initially
            memset(counts, 0, sizeof(counts));
            if(count_usage(argv[i + 2], counts) != 0) {
                fprintf(stderr, "Error counting usage in file %s\n", argv[i + 2]);
            }
            else {
                // lock thread so we can write to output file
                pthread_mutex_lock(&mutex1);

                // open file to append
                of = fopen(out_file, "a");

                // if file exists, write results to it
                if(of > 0) {
                    fprintf(of, "********* Results of %s *********\n", argv[i + 2]);
                    for(int i = 0; i < NUM_CHAR; i++) {
                        fprintf(of, "%c: %d\n", i + 97, counts[i]);
                    }
                    fprintf(of, "\n");
                    fclose(of);
                }
                else {
                    fprintf(stderr, "Error opening output file\n");
                }
                pthread_mutex_unlock(&mutex1);
            }

            // decrease thread count
            pthread_mutex_lock(&mutex1);
            counter--;
            pthread_mutex_unlock(&mutex1);

            // break for loop since thread has done its job
            break;
        }
    }

    // wait for all threads to finish execution
    while(counter > 0) ;

    // if pid non zero we are in main thread. Print goodbye message
    if(pid)
        printf("All threads finished! Terminating program\n");

    return 0;
}
