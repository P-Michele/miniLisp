cmake_minimum_required(VERSION 3.30)
project(miniLisp C)

set(CMAKE_C_STANDARD 11)

add_executable(miniLisp
        main.c
        mpc.c
)
