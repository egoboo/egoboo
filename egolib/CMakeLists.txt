# Minimum required CMake version.
cmake_minimum_required(VERSION 3.6)

# Project name and settings.
project(egolib-library CXX)
message("building Egolib")

add_subdirectory(library)

add_subdirectory(tests)
