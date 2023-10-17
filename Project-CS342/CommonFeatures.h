//
// Created by DELL on 16.10.2023.
//

#ifndef PROJECT_CS342_COMMONFEATURES_H
#define PROJECT_CS342_COMMONFEATURES_H
#include <stdbool.h>

// Checking if the number is prime or not.
bool IsPrimeNumber(const int num);
// Parsing the command line arguments and reading the flag values for the rest of the program.
void commandLineParsing(int argc, char *argv[], int *flag_n, int *flag_m, char *flag_i, char *flag_o);

#endif //PROJECT_CS342_COMMONFEATURES_H
