//
// Created by DELL on 16.10.2023.
//
#include "CommonFeatures.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

bool IsPrimeNumber(const int num){
    if (num < 2) return false;
    for (int i = 2; i <= sqrt(num); i++) {
        if (num % i == 0){
            return false;
        }
    }
    return true;
}

// Parsing the command line arguments and reading the flag values for the rest of the program.
void commandLineParsing(int argc, char *argv[], int *flag_n, int *flag_m, char *flag_i, char *flag_o){
    int n_val = 5; // Default value of child processes
    int m_val = 3; // Default value of primes per message
    char *in_filename = NULL;
    char *out_filename = NULL;

    // Passing "n:m:i:o:" flags one by one.
    // arg_flag is the flag in the command line and optarg is the flag value in string format.
    int arg_flag;
    while ((arg_flag = getopt(argc, argv, "n:m:i:o:")) != -1) {
        switch (arg_flag) {
            case 'n':
                n_val = atoi(optarg);
                if (n_val < 1 || n_val > 20) {
                    fprintf(stderr, "Invalid value for -n. It should be in the range [1, 20].\n");
                    // If the input is invalid, reinitialize the default value.
                    n_val = 5;
                    return;
                }
                break;
            case 'm':
                m_val = atoi(optarg);
                if (m_val < 1 || m_val > 20) {
                    fprintf(stderr, "Invalid value for -m. It should be in the range [1, 20].\n");
                    // If the input is invalid, reinitialize the default value.
                    m_val = 3;
                    return;
                }
                break;
            case 'i':
                in_filename = optarg;
                break;
            case 'o':
                out_filename = optarg;
                break;
            default:
                return;
        }
    }
    *flag_n = n_val;
    *flag_m = m_val;
    flag_i = in_filename;
    flag_o = out_filename;
}
