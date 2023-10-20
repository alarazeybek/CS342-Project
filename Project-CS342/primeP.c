//
// Created by DELL on 16.10.2023.
//
#include "CommonFeatures.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h> //silinebilir
#include <string.h>
// Includes for the process handling
#include <unistd.h>
#include <sys/types.h>
// Message includes
#include <mqueue.h>
#include <errno.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

struct item {
    int prime_num;
};
// Message queue icin gerekenler:
struct item *itemp;
char *bufferp; // send buffer
int   bufferlen; // buffer size  (in bytes)
mqd_t mq; // message_queue

struct mq_attr attr; // message_attributes

void ProcessHandling(const int p_child_num, const int message_size, char* intermediate_files[], const char* output_file_name );

int main(int argc, char *argv[]){
    // Command Line Parsing:

    char* n_val[100] ;
    char* m_val[100];
    char* input_file_name[100];
    char* output_file_name[100];
    commandLineParsing(argc,argv,n_val,m_val,input_file_name,
                       output_file_name);

    int child_process_num = atoi(*n_val);
    int prime_num_in_message = atoi(*m_val);
    printf ("\nN_vall: %d",child_process_num);
    printf ("\nM_vall: %d",prime_num_in_message);

    // Opening a Message Queue:
    attr.mq_maxmsg = prime_num_in_message;
    attr.mq_curmsgs = 0;
    bufferlen = sizeof(struct item)*sizeof(struct mq_attr);
    mq = mq_open("/messagequeue", O_RDWR | O_CREAT,  0666, NULL);
    if (mq == -1) {
        perror("FLAG:can not open msg queue\n");
        exit(1);
    }

    printf ("\nF1");
    mq_getattr(mq, &attr);
    printf ("\nF2");
    bufferp = (char *) malloc(bufferlen);
    printf ("\nF2.2222");


    // Parsing the input file into N intermediate input files.

    char* inter_files[child_process_num];
    for (int i = 0; i < child_process_num; i++) {
        inter_files[i] = (char *)malloc(100); // Allocate memory for each element
        if (inter_files[i] == NULL) {
            perror("malloc"); // Check for allocation failure
            exit(1);
        }
    }
    printf ("\nF3");

    openIntermediateFiles(*input_file_name, inter_files, child_process_num);
    printf ("\nF4");
    ProcessHandling(child_process_num,prime_num_in_message, inter_files, *output_file_name);
    printf ("\nF5");
    //DeleteIntermediateFiles(child_process_num);

    free(bufferp);
    for (int i = 0; i < child_process_num; i++) {
        free(inter_files[i]); // Free memory for each element
    }
    mq_close(mq);
    return 0;
}

void ProcessHandling(const int p_child_num, const int message_size, char* inter_files[], const char* output_file_name){
    FILE* f_write = fopen(output_file_name, "w+");
    if (f_write == NULL) {
        perror("Error opening file");
        printf("errno: %d\n", errno);
        return ;
    }
    pid_t  n;
    int MCOUNT = 0;
    printf("Message Size: %d", message_size);
    for (int a1 = 0; a1 < p_child_num; a1++) {
        n = fork();
        if (n < 0) {
            printf ("fork() failed\n");
            exit (1);
        }
        // If n is 0, it means that we are running the child process.
        if (n == 0) {
            char* inter_file_name = inter_files[a1];
            FILE *inter_file = fopen(inter_file_name, "r");
            if (inter_file == NULL) {
                perror("Error opening intermediate file");
                exit(1);
            }
            if (inter_file == NULL) {
                perror("Error opening file");
                printf("errno: %d\n", errno);
                return ;
            }

            char line[100]; // Assuming a maximum of 100 characters per line
            while (fgets(line, sizeof(line), inter_file) != NULL) {
                // Convert the line to an integer
                int number = atoi(line);
                if (IsPrimeNumber(number)){
                    // Create message
                    itemp = (struct item *) bufferp;
                    itemp->prime_num = number;
                    printf ("\nIsPrime Inside:%d",number);
                    int error = mq_send(mq, bufferp, sizeof(struct item), 0);
                    if (error == -1) {
                        perror("mq_send failed\n");
                        exit(1);
                    }
                    MCOUNT++;
                }
            }
            if (ferror(inter_file)) {
                perror("Error reading from the intermediate file");
            }
            fclose(inter_file);
            exit(0);  // child terminates
        }
        else {
            printf ("\nELSEE");
            // this is parent code
            // mesage queue size çek et eğet m değerinden büyük ise ana output file ına yazdır ve mesajı queue sunu boşalt.
            if (MCOUNT >= message_size) {
                printf ("\nELSEE ICI");
                while (MCOUNT > 0) {
                    printf ("\nYAZİYORUMMM");
                    int error = mq_receive(mq, bufferp, bufferlen, NULL);
                    if (error == -1) {
                        perror("mq_receive failed\n");
                        exit(1);
                    }
                    itemp = (struct item *) bufferp;
                    fprintf(f_write, "%d\n", itemp->prime_num);
                    MCOUNT--;
                }
            }
        }
    }
    // wait for all children to terminate.
    for (int i = 0; i < p_child_num; i++) {
        wait(NULL);
    }
    // Handling the left-overs:
    while (MCOUNT > 0) {
        int error = mq_receive(mq, bufferp, bufferlen, NULL);
        if (error == -1) {
            perror("mq_receive failed\n");
            exit(1);
        }
        itemp = (struct item *) bufferp;
        fprintf(f_write, "%d\n", itemp->prime_num);
        MCOUNT--;
    }
    // Close output file.
    fclose(f_write);
}