cmake_minimum_required(VERSION 3.12.4)


file(GLOB_RECURSE SOURCES ../lvgl/demos/*.c ../lvgl/examples/*.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS .
                       REQUIRES lvgl)
 
