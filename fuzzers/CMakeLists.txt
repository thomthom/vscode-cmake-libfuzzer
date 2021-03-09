# Allow short runs during automated testing to see if something new breaks
set(FUZZ_RUNTIME 10
  CACHE STRING "Number of seconds to run fuzz tests during ctest run"
) # Default of 10 seconds

set(CAN_FUZZ NO)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(WARNING "Cannot enable fuzzing. Unsupported compiler: ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_ID})")
  return()
endif()

# Interface for linking to Clangs libraries.
add_library(clang_fuzzer INTERFACE)

# https://llvm.org/docs/LibFuzzer.html#fuzzer-usage

if(WIN32)
  # Pick up the configured clang executable and not the one on PATH.
  # The CMAKE_CXX_COMPILER might be clang-cl.exe which doesn't support the
  # -print-search-dirs argument, so we fetch it's directory to locate clang.exe.
  message(DEBUG "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
  get_filename_component(CLANG_BIN_DIRECTORY ${CMAKE_CXX_COMPILER} DIRECTORY)
  message(DEBUG "CLANG_BIN_DIRECTORY: ${CLANG_BIN_DIRECTORY}")
  set(CLANG_EXE "${CLANG_BIN_DIRECTORY}/clang.exe")
  message(DEBUG "CLANG_EXE: ${CLANG_EXE}")
  # Find Clang's libraries.
  #
  # Example output (Clang 11)
  #   $> clang -print-search-dirs
  #   programs: =C:\Program Files\LLVM\bin
  #   libraries: =C:\Program Files\LLVM\lib\clang\11.0.0
  execute_process(COMMAND ${CLANG_EXE} -print-search-dirs
    OUTPUT_VARIABLE CLANG_OUTPUT
    ERROR_VARIABLE CLANG_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULTS_VARIABLE CLANG_RESULTS
  )
  message(DEBUG "CLANG_OUTPUT: ${CLANG_OUTPUT}")
  message(DEBUG "CLANG_ERROR: ${CLANG_ERROR}")
  message(DEBUG "CLANG_RESULTS: ${CLANG_RESULTS}")

  if(NOT CLANG_RESULTS EQUAL 0)
    message(WARNING "Unable to locate Clang Search Directories: ${CLANG_RESULTS}")
  endif()

  string(REGEX MATCH "libraries: =(.+)" _MATCH "${CLANG_OUTPUT}")
  message(DEBUG "CMAKE_MATCH_1: ${CMAKE_MATCH_1}")
  set(CLANG_LIBRARIES_DIR ${CMAKE_MATCH_1})
  message(DEBUG "CLANG_LIBRARIES_DIR: ${CLANG_LIBRARIES_DIR}")

  function(find_clang_library VAR LIBRARY_NAME)
    find_library("_${VAR}" ${LIBRARY_NAME}
      HINTS ${CLANG_LIBRARIES_DIR}
      PATH_SUFFIXES "lib/windows"
      NO_DEFAULT_PATH
    )
    set(${VAR} ${_${VAR}} PARENT_SCOPE)
  endfunction()

  find_clang_library(CLANG_RT_ASAN_PREINIT "clang_rt.asan-preinit-x86_64")
  find_clang_library(CLANG_RT_ASAN "clang_rt.asan-x86_64")
  find_clang_library(CLANG_RT_ASAN_CXX "clang_rt.asan_cxx-x86_64")
  find_clang_library(CLANG_RT_FUZZER "clang_rt.fuzzer-x86_64")
  message(DEBUG "CLANG_RT_ASAN_PREINIT: ${CLANG_RT_ASAN_PREINIT}")
  message(DEBUG "CLANG_RT_ASAN: ${CLANG_RT_ASAN}")
  message(DEBUG "CLANG_RT_ASAN_CXX: ${CLANG_RT_ASAN_CXX}")
  message(DEBUG "CLANG_RT_FUZZER: ${CLANG_RT_FUZZER}")

  if(CLANG_RT_ASAN_PREINIT AND CLANG_RT_ASAN AND CLANG_RT_ASAN_CXX AND CLANG_RT_FUZZER)
    set(CAN_FUZZ YES)
  endif()

  target_link_libraries(clang_fuzzer INTERFACE
    "${CLANG_RT_ASAN_PREINIT}"
    "${CLANG_RT_ASAN}"
    "${CLANG_RT_ASAN_CXX}"
    "${CLANG_RT_FUZZER}"
  )

else()

  target_link_libraries(clang_fuzzer INTERFACE
    -fsanitize=fuzzer,address
  )

endif()

if(CAN_FUZZ)
  target_compile_options(clang_fuzzer INTERFACE
    -fsanitize=fuzzer,address
  )
endif()

message(STATUS "Can fuzz: ${CAN_FUZZ}")
if(NOT CAN_FUZZ)
  message(WARNING "Cannot enable fuzzing. Dependencies not found. Ensure a recent version of Clang is available.")
endif()

function(add_fuzzer SOURCE)
  get_filename_component(TARGET_NAME ${SOURCE} NAME_WLE)
  message(STATUS "Adding fuzzer: ${TARGET_NAME}")

  add_executable(${TARGET_NAME} ${SOURCE})

  target_link_libraries(${TARGET_NAME} PRIVATE
    clang_fuzzer
  )

  add_test(NAME "${TARGET_NAME}_run"
    CONFIGURATIONS Release
    COMMAND ${TARGET_NAME} -max_total_time=${FUZZ_RUNTIME} "${CMAKE_CURRENT_LIST_DIR}/corpus/${TARGET_NAME}/"
  )
  set_tests_properties("${TARGET_NAME}_run" PROPERTIES LABELS "Fuzz")

endfunction()

if(CAN_FUZZ)
  add_fuzzer(fuzz_test.cpp)
endif()
