cmake_minimum_required(VERSION 3.25)
project(Project_CS342)

set(CMAKE_CXX_STANDARD 17)

add_executable(Project_CS342 main.cpp CommandLineParsing.h CommandLineParsing.c CommonFeatures.c CommonFeatures.h primeP.c primeT.c)
