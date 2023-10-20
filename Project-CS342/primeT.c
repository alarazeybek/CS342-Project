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
    struct LinkedList* own_list;
};
static void *do_task(void *arg_ptr);
int main(int argc, char* argv[]) {
    // Command Line Parsing:
    char* n_val[100] ;
    char* m_val[100];
    char* input_file_name[100];
    char* output_file_name[100];

    commandLineParsing(argc,argv,n_val,m_val,input_file_name,
                       output_file_name);
    int child_thread_num = atoi(*n_val);

    // Split the input file into N intermediate input files.
    char* inter_files[child_thread_num];
    for (int i = 0; i < child_thread_num; i++) {
        inter_files[i] = (char *)malloc(100); // Allocate memory for each element
        if (inter_files[i] == NULL) {
            perror("malloc"); // Check for allocation failure
            exit(1);
        }
    }
    openIntermediateFiles(*input_file_name, inter_files, child_thread_num);

    // Create an array of thread handles and thread data.
    pthread_t tids[child_thread_num];
    struct ThreadData t_args[child_thread_num];
    char *retmsg;
    // Create and start worker threads.
    for (int i = 0; i < child_thread_num; i++) {
        t_args[i].interFileName = inter_files[i];/* assign intermediate file name */;
        t_args[i].threadNumber = i;
        t_args[i].own_list = createLinkedList();
        int ret = pthread_create(&(tids[i]), NULL, do_task, (void*)&t_args[i]);
        if (ret != 0) {
            printf("thread create failed \n");
            exit(1);
        }
    }

    // Wait for all threads to finish.
    for (int i = 0; i < child_thread_num; i++) {
        int ret = pthread_join(tids[i], (void **)&retmsg); //
        if (ret != 0) {
            printf("thread join failed \n");
            exit(1);
        }
        free (retmsg);
    }

    // Collect and print prime numbers to the output file (OUTFILE).
    FILE* f_write = fopen(*output_file_name, "w+");
    for (int m = 0; m<child_thread_num;m++){
        while((t_args[m].own_list)->head != NULL) {
            fprintf(f_write, "%d\n", GetPrimeNum(t_args[m].own_list));
            DeleteNode(t_args[m].own_list);
        }
        free(t_args[m].own_list);
    }
    fclose(f_write);
    // Remove intermediate files.

    for (int i = 0; i < child_thread_num; i++) {
        // Use remove() to delete the file
        if (remove(inter_files[i]) != 0) {
            perror("Error deleting file");
        }
    }
    return 0;
}

// this is the function to be executed by all the threads concurrently
static void *do_task(void *arg_ptr){
    char *retreason;

    FILE *fp = fopen(((struct ThreadData *) arg_ptr)->interFileName, "r");
    if (fp == NULL) {
        perror("do_task:");
        exit(1);
    }

    char line[100];
    while (fgets(line, sizeof(line), fp)) {
        int number = atoi(line);
        if (IsPrimeNumber(number)){
            insert(((struct ThreadData *) arg_ptr)->own_list, number);
        }
    }

    fclose(fp);

    retreason = (char *) malloc (200); //
    strcpy (retreason, "normal termination of thread");
    pthread_exit(retreason); //  tell a reason to thread waiting in join
}