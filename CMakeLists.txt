cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(FuzzyDunlop
  VERSION 1.0.0
  DESCRIPTION "libFuzzer playground"
  LANGUAGES C CXX
)

add_subdirectory(fuzzers)
