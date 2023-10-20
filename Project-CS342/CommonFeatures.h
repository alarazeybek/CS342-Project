//
// Created by DELL on 16.10.2023.
//

#ifndef PROJECT_CS342_COMMONFEATURES_H
#define PROJECT_CS342_COMMONFEATURES_H
#include <stdbool.h>
#include <string.h>

// Checking if the number is prime or not.
bool IsPrimeNumber(const int num);
// Parsing the command line arguments and reading the flag values for the rest of the program.
void commandLineParsing(int argc, char *argv[], string *flag_n, string *flag_m, char *flag_i, char *flag_o);
// Parsing the inout file into  N inter files.
void openIntermediateFiles(char *inputFileName, char* inter_files[], const int child_process_num);
// Deleting the inter files.
void DeleteIntermediateFiles(const int child_process_num);

#endif //PROJECT_CS342_COMMONFEATURES_H
