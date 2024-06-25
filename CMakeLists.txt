cmake_minimum_required(VERSION 3.28)
project(TM3SO C)

set(CMAKE_C_STANDARD 11)

add_executable(TM3SO fat16/src/main.c
        fat16/src/fat16.h
        fat16/src/fat16.c
        fat16/src/commands.h
        fat16/src/commands.c
        fat16/src/output.h
        fat16/src/output.c
        fat16/src/support.h
        fat16/src/support.c
)
