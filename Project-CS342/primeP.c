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
    bufferlen = attr.mq_maxmsg + 10;
    mq = mq_open("/messagequeue", O_RDWR | O_CREAT, /*QUEUE_PERMISSIONS*/ 0666, NULL);
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
    }
    printf ("\nF3");

    openIntermediateFiles(*input_file_name, inter_files, child_process_num);
    printf ("\nF4");
    ProcessHandling(child_process_num,prime_num_in_message, inter_files, *output_file_name);
    printf ("\nF5");
    DeleteIntermediateFiles(child_process_num);
    printf ("\nF6");
    free(bufferp);
    for (int i = 0; i < child_process_num; i++) {
        free(inter_files[i]); // Free memory for each element
    }
    mq_close(mq);
    return 0;
}

void ProcessHandling(const int p_child_num, const int message_size, char* inter_files[], const char* output_file_name)
{
    FILE* f_write = fopen(output_file_name, "w+");
    pid_t  n;
    for (int i = 0; i < p_child_num; ++i) {
        n = fork();
        if (n < 0) {
            printf ("fork() failed\n");
            exit (1);
        }
        // If n is 0, it means that we are running the child process.
        if (n == 0) {
            char* inter_file_name = inter_files[i];
            FILE *inter_file = fopen(inter_file_name, "r");
            // Reading the intermediate file:
            char line[100]; // Assuming a maximum of 100 characters per line
            while (fgets(line, sizeof(line), inter_file) != NULL) {
                // Convert the line to an integer
                int number = atoi(line);
                if (IsPrimeNumber(number)){
                    // Create message
                    itemp = (struct item *) bufferp;
                    itemp->prime_num = number;
                    int error = mq_send(mq, bufferp, sizeof(struct item), 0);
                    if (error == -1) {
                        perror("mq_send failed\n");
                        exit(1);
                    }
                }
            }
            exit(0);  // child terminates
        }
        else {
            // this is parent code
            // mesage queue size çek et eğet m değerinden büyük ise ana output file ına yazdır ve mesajı queue sunu boşalt.
            if (attr.mq_curmsgs >= message_size) {
                while (attr.mq_curmsgs > 0) {
                    int error = mq_receive(mq, bufferp, bufferlen, NULL);
                    if (error == -1) {
                        perror("mq_receive failed\n");
                        exit(1);
                    }
                    itemp = (struct item *) bufferp;
                    fprintf(f_write, "%d\n", itemp->prime_num);
                }
            }
        }
    }
    // wait for all children to terminate.
    for (int i = 0; i < p_child_num; i++) {
        wait(NULL);
    }
    // Handling the left-overs:
    if (attr.mq_curmsgs > 0) {
        int error = mq_receive(mq, bufferp, bufferlen, NULL);
        if (error == -1) {
            perror("mq_receive failed\n");
            exit(1);
        }
        itemp = (struct item *) bufferp;
        fprintf(f_write, "%d\n", itemp->prime_num);
    }
    // Close output file.
    fclose(f_write);
}