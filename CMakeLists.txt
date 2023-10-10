cmake_minimum_required(VERSION 3.0)
project(GTKWindow)

set(CMAKE_CXX_STANDARD 11) 

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

set(SOURCES
    GTKWindow.cpp
)

add_executable(GTKWindow ${SOURCES})

target_include_directories(GTKWindow PRIVATE ${GTK3_INCLUDE_DIRS} ${GST_INCLUDE_DIRS})

target_link_libraries(GTKWindow PRIVATE ${GTK3_LIBRARIES} ${GST_LIBRARIES})

install(TARGETS GTKWindow DESTINATION bin)
