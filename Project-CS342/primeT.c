//
// Created by DELL on 20.10.2023.
//
#include "CommonFeatures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
// Define the structure for a single node in the linked list.
struct Node {
    int prime_num;
    int frequency;
    struct Node* next;
};

// Define the structure for the linked list.
struct LinkedList {
    struct Node* head;
};

// Function to create a new linked list.
struct LinkedList* createLinkedList() {
    struct LinkedList* newList = (struct LinkedList*)malloc(sizeof(struct LinkedList));
    newList->head = NULL;
    return newList;
}

// Function to insert a new node at the beginning of the linked list.
void insert(struct LinkedList* list, int number) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->prime_num = number;
    newNode->next = list->head;
    list->head = newNode;
}
// Get the prime number of the head.
int GetPrimeNum(struct LinkedList* list) {
    return list->head->prime_num;
}
// Get the prime number of the head.
int GetFrequency(struct LinkedList* list) {
    return list->head->frequency;
}
// Function to free the memory of the linked list.
void DeleteNode(struct LinkedList* list) {
    struct Node* f_item = list->head;
    list->head = f_item->next;
    free(f_item);
}
// Define a struct to pass data to the worker threads.
struct ThreadData {
    char* inputFileName;
    int threadNumber;
    //struct LinkedList* link;
    //pthread_mutex_t* mutex;
};
// LinkedList

LinkedList* PrimeList;

int main(int argc, char* argv[]) {
    PrimeList = createLinkedList();
    // Command Line Parsing:
    int *child_thread_num = (int*)malloc(sizeof(int);
    int *prime_num_in_message = (int*)malloc(sizeof(int);
    char input_file_name[100];
    char output_file_name[100];
    commandLineParsing(argc,argv,*child_thread_num,*prime_num_in_message,input_file_name,
                       output_file_name);

    // Split the input file into N intermediate input files.
    char* inter_files[*child_process_num];
    openIntermediateFiles(input_file_name, inter_files, *child_thread_num_num);
    // Create an array of thread handles and thread data.
    pthread_t tids[*child_thread_num];
    struct ThreadData t_args[*child_thread_num];
    char *retmsg;
/*  Chat bunlari ekleyerek olusturuyor:
    // Create a mutex for synchronizing thread access to shared data.
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
*/
    // Create and start worker threads.
    for (int i = 0; i < *child_thread_num; i++) {
        t_args[i].inputFileName = inter_files[i];/* assign intermediate file name */;
        t_args[i].threadNumber = i;
        //t_args[i].primeList = /* create a thread-specific linked list */;
        //t_args[i].mutex = &mutex;
        int ret = pthread_create(&(tids[i]), NULL, do_task, (void*)&t_args[i]);
        if (ret != 0) {
            printf("thread create failed \n");
            exit(1);
        }
    }

    // Wait for all threads to finish.
    for (int i = 0; i < *child_thread_num; i++) {
        int ret = pthread_join(tids[i], (void **)&retmsg);
        if (ret != 0) {
            printf("thread join failed \n");
            exit(1);
        }
        free (retmsg);
    }

    // Collect and print prime numbers to the output file (OUTFILE).
    FILE* f_write = fopen(output_file_name, "w+");
    while(PrimeList->head != NULL){
        fprintf(f_write, "%d\n", GetPrimeNum(PrimeList));
        DeleteNode(PrimeList);
    }
    free(PrimeList);
    fclose(f_write);
    // Remove intermediate files.
    DeleteIntermediateFiles(*child_thread_num);

    //pthread_mutex_destroy(&mutex);

    return 0;
}

// this is the function to be executed by all the threads concurrently
static void *do_task(void *arg_ptr)
{
    int i;
    FILE *fp;
    char filename[MAXFILENAME];
    char *retreason;

    printf("thread %d started\n", ((struct arg *) arg_ptr)->t_index);

    sprintf(filename, "output_of_thread%d.txt",
            ((struct arg *) arg_ptr)->t_index);

    fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("do_task:");
        exit(1);
    }

    for (i = ((struct arg *) arg_ptr)->n;
         i <= ((struct arg *) arg_ptr)->m; ++i) {
        fprintf(fp, "integer = %d\n", i);
    }

    fclose(fp);

    retreason = (char *) malloc (200);
    strcpy (retreason, "normal termination of thread");
    pthread_exit(retreason); //  tell a reason to thread waiting in join
    // we could simple exit as below, if we don't want to pass a reason
    // pthread_exit(NULL);
    // then we would also modify pthread_join call.
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