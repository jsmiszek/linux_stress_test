cmake_minimum_required(VERSION 3.5)

project(projekt3 LANGUAGES C)

add_executable(massivereader massivereader.c massivereader.h helper.c helper.h)
add_executable(multiwriter multiwriter.c multiwriter.h helper.c helper.h)
