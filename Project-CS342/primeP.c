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
void openIntermediateFiles(char *inputFileName, char* inter_files[], const int child_process_num);
void DeleteIntermediateFiles(FILE* inter_files, const int child_process_num);
int main(int argc, char *argv[]){
    int *child_process_num, *prime_num_in_message;
    char *input_file_name, *output_file_num;
    commandLineParsing(argc,argv,child_process_num,prime_num_in_message,input_file_name,
                       output_file_num);
    // parsing the input file into N intermediate input files.
    char* inter_files[*child_process_num];


    int inter_files_index;
    openIntermediateFiles();
   // chlidprocessolustur()
    DeleteIntermediateFiles();
    return 0;
}
// open file 


void DeleteIntermediateFiles(FILE* inter_files, const int child_process_num) {
    for (int i = 1; i <= child_process_num; i++) {
        char filename[25];
        snprintf(filename, sizeof(filename), "%d.txt", i);
        // Use remove() to delete the file
        if (remove(filename) != 0) {
            perror("Error deleting file");
        }
    }
}
void openIntermediateFiles(char *inputFileName, char* inter_files[], const int child_process_num){
    FILE *f_input;
    int main_input_file_index = 0;
    f_input = fopen(inputFileName, "r");
    if (f_input == NULL) {
        exit(EXIT_FAILURE);
    }
    // interleri olustur: Isimler 1 den baslar ve 1,2,3,4... diye gider
    for (int intermediate_file_no = 1; intermediate_file_no <= child_process_num; intermediate_file_no++){
        char inter_file_name[25]; // TODO Check et
        snprintf(inter_file_name, sizeof(inter_file_name), "%d", intermediate_file_no);
        FILE *f_write = fopen(inter_file_name, "w+");
        if (f_write == NULL) {
            perror("File create/open failed");
            continue;
        }
        inter_files[intermediate_file_no] = inter_file_name;
        fclose(f_write);
    }
    // ana input file ını oku ve interleri initialize et:
    char line[100]; // Assuming a maximum of 100 characters per line
    int line_index = 1;
    while (fgets(line, sizeof(line), f_input) != NULL) {
        // Convert the line to an integer
        int number = atoi(line);
        // If the line is an integer:
        if (number != 0 || line[0] == '0') { // TODO o inputunu dene
            int inter_file_array_index = (line_index -1) % child_process_num + 1;
            char* inter_file_name = inter_files[inter_file_array_index];
            FILE* inter_file = fopen(inter_file_name, "r");
            fprintf(inter_file, "%d\n", number);
            fclose(inter_file);
        }
        line_index++;
    }
    fclose(f_input); // Close the input file when done
}


void ProcessHandling(const int p_child_num, const int message_size, char* inter_files[] )
{
    pid_t  n;
    printf ("this is parent with pid is: %d\n", getpid());
    int k = 0;
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
                }
            }
            exit(0);  // child terminates
        }
        else {
            // this is parent code
            // mesage queue size çek et eğet m değerinden büyük ise ana output file ına yazdır ve mesajı queue sunu boşalt.

        }
    }
    // wait for all children to terminate.
    for (int i = 0; i < p_child_num; i++) {
        wait(NULL);
    }
    // printf ("all children terminated. bye... \n");
}