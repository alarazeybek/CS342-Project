//
// Created by DELL on 16.10.2023.
//
#include "CommonFeatures.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

bool IsPrimeNumber(const int num){
    if (num < 2) return false;

    for (int i = 2; i < num; i++) {
        if (num % i == 0){
            return false;
        }
    }
    return true;
}

    // Parsing the command line arguments and reading the flag values for the rest of the program.
    void commandLineParsing(int argc, char *argv[],  char *flag_n[100],  char *flag_m[100], char *flag_i, char *flag_o){
        int n_val = 5; // Default value of child processes
        int m_val = 3; // Default value of primes per message
        char *in_filename = NULL;
        char *out_filename = NULL;

        // Passing "n:m:i:o:" flags one by one.
        // arg_flag is the flag in the command line and optarg is the flag value in string format.
        int arg_flag;
        while ((arg_flag = getopt(argc, argv, "n:m:i:o:")) != -1) {
            printf("Char:%c\n",arg_flag);
            switch (arg_flag) {
                printf("Optarg:%s\n",optarg);
                case 'n':
                    printf("Optarg Inside:%s\n",optarg);
                    n_val = atoi(optarg);
                    printf("N_val Inside:%d\n",n_val);
                    if (n_val < 1 || n_val > 20) {
                        fprintf(stderr, "Invalid value for -n. It should be in the range [1, 20].\n");
                        // If the input is invalid, reinitialize the default value.
                        n_val = 5;
                        return;
                    }
                    break;
                case 'm':
                    printf("Optarg Inside:%s\n",optarg);
                    m_val = atoi(optarg);
                    printf("M_val Inside:%d\n",m_val);
                    if (m_val < 1 || m_val > 20) {
                        fprintf(stderr, "Invalid value for -m. It should be in the range [1, 20].\n");
                        // If the input is invalid, reinitialize the default value.
                        m_val = 3;
                        return;
                    }
                    break;
                case 'i':
                    printf("Optarg Inside:%s\n",optarg);
                    in_filename = optarg;
                    break;
                case 'o':
                    printf("Optarg Inside:%s\n",optarg);
                    out_filename = optarg;
                    break;
                default:
                    return;
            }
        }
        char string_flag_val[100];
        snprintf(string_flag_val, sizeof(string_flag_val), "%d", n_val);
        *flag_n = string_flag_val;
        snprintf(string_flag_val, sizeof(string_flag_val), "%d", m_val);
        *flag_m = string_flag_val;
        flag_i = in_filename;
        flag_o = out_filename;
    }


void DeleteIntermediateFiles(const int child_process_num) {
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