//
// Created by DELL on 20.10.2023.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

// Define a struct to pass data to the worker threads.
struct ThreadData {
    char* inputFileName;
    int threadNumber;
    struct PrimeData* primeList;
    pthread_mutex_t* mutex;
};

int main(int argc, char* argv[]) {
    // Parse command line arguments to get N, INFILE, and OUTFILE.
    // ...

    // Split the input file into N intermediate input files.
    // ...

    // Create an array of thread handles and thread data.
    pthread_t threads[N];
    struct ThreadData threadDataArray[N];

    // Create a mutex for synchronizing thread access to shared data.
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // Create and start worker threads.
    for (int i = 0; i < N; i++) {
        threadDataArray[i].inputFileName = /* assign intermediate file name */;
        threadDataArray[i].threadNumber = i;
        threadDataArray[i].primeList = /* create a thread-specific linked list */;
        threadDataArray[i].mutex = &mutex;
        pthread_create(&threads[i], NULL, ProcessIntermediateFile, (void*)&threadDataArray[i]);
    }

    // Wait for all threads to finish.
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    // Collect and print prime numbers to the output file (OUTFILE).
    // ...

    // Remove intermediate files.
    // ...

    pthread_mutex_destroy(&mutex);

    return 0;
}

// Thread function to process an intermediate file.
void* ProcessIntermediateFile(void* data) {
    struct ThreadData* threadData = (struct ThreadData*)data;

    FILE* inputFile = fopen(threadData->inputFileName, "r");
    if (!inputFile) {
        perror("Error opening input file");
        pthread_exit(NULL);
    }

    char line[100];
    while (fgets(line, sizeof(line), inputFile)) {
        int number = atoi(line);
        if (IsPrime(number)) {
            pthread_mutex_lock(threadData->mutex);
            // Update the prime list with the prime number.
            // You can also implement frequency counting here.
            // Make sure to use the mutex to protect the list.
            // ...
            pthread_mutex_unlock(threadData->mutex);
        }
    }

    fclose(inputFile);
    pthread_exit(NULL);
}

