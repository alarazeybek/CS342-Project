//
// Created by DELL on 20.10.2023.
//
#include "CommonFeatures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/wait.h>
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
    char* interFileName;
    int threadNumber;
    //struct LinkedList* link;
    //pthread_mutex_t* mutex;
};
// LinkedList

struct LinkedList* PrimeList;
static void *do_task(void *arg_ptr);
int main(int argc, char* argv[]) {
    printf("F1 LINE \n");
    PrimeList = createLinkedList();
    // Command Line Parsing:
    char* n_val[100] ;
    char* m_val[100];
    char* input_file_name[100];
    char* output_file_name[100];
    printf("F2 LINE \n");
    commandLineParsing(argc,argv,n_val,m_val,input_file_name,
                       output_file_name);
    int child_thread_num = atoi(n_val);
    printf("F3 LINE \n");
    // int prime_num_in_message = atoi(m_val); BU PART B Kisminda kullanilmiyor.

    // Split the input file into N intermediate input files.
    char* inter_files[child_thread_num];
    for (int i = 0; i < child_thread_num; i++) {
        inter_files[i] = (char *)malloc(100); // Allocate memory for each element
        if (inter_files[i] == NULL) {
            perror("malloc"); // Check for allocation failure
            exit(1);
        }
    }
    printf("00000000000000000000 LINE \n");
    openIntermediateFiles(*input_file_name, inter_files, child_thread_num);
    printf("F4 LINE \n");
    // Create an array of thread handles and thread data.
    pthread_t tids[child_thread_num];
    struct ThreadData t_args[child_thread_num];
    char *retmsg;
/*  Chat bunlari ekleyerek olusturuyor:
    // Create a mutex for synchronizing thread access to shared data.
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
*/
    printf("F5 LINE \n");
    // Create and start worker threads.
    for (int i = 0; i < child_thread_num; i++) {
        t_args[i].interFileName = inter_files[i];/* assign intermediate file name */;
        t_args[i].threadNumber = i;
        //t_args[i].primeList = /* create a thread-specific linked list */;
        //t_args[i].mutex = &mutex;
        int ret = pthread_create(&(tids[i]), NULL, do_task, (void*)&t_args[i]);
        if (ret != 0) {
            printf("thread create failed \n");
            exit(1);
        }
    }

    printf("F6 LINE \n");
    // Wait for all threads to finish.
    for (int i = 0; i < child_thread_num; i++) {
        int ret = pthread_join(tids[i], (void **)&retmsg);
        if (ret != 0) {
            printf("thread join failed \n");
            exit(1);
        }
        free (retmsg);
    }

    // Collect and print prime numbers to the output file (OUTFILE).
    FILE* f_write = fopen(*output_file_name, "w+");
    printf("OUTPUT OPEN OLMALI \n");
    while(PrimeList->head != NULL){
        fprintf(f_write, "%d\n", GetPrimeNum(PrimeList));
        DeleteNode(PrimeList);
    }
    free(PrimeList);
    fclose(f_write);
    // Remove intermediate files.
    DeleteIntermediateFiles(child_thread_num);
    printf("AT THE END OF MAIN \n");
    //pthread_mutex_destroy(&mutex);

    /*
    for (int i = 0; i < child_thread_num; i++) {
        // Use remove() to delete the file
        if (remove(inter_files[i]) != 0) {
            perror("Error deleting file");
        }
    }
     */
    return 0;
}

// this is the function to be executed by all the threads concurrently

static void *do_task(void *arg_ptr)
{
    char *retreason;

    FILE *fp = fopen(((struct ThreadData *) arg_ptr)->interFileName, "w");
    if (fp == NULL) {
        perror("do_task:");
        exit(1);
    }

    char line[100];
    while (fgets(line, sizeof(line), fp)) {
        // pthread_mutex_lock(threadData->mutex);
        int number = atoi(line);
        if (IsPrimeNumber(number)){
            insert(PrimeList, number);
            printf("DO TASK INSIDEEE \n");
        }
        //pthread_mutex_unlock(threadData->mutex);
    }

    fclose(fp);
    printf("At THe ENDDD OF DOOO \n");
    retreason = (char *) malloc (200);
    strcpy (retreason, "normal termination of thread");
    pthread_exit(retreason); //  tell a reason to thread waiting in join
    // we could simple exit as below, if we don't want to pass a reason
    // pthread_exit(NULL);
    // then we would also modify pthread_join call.
}