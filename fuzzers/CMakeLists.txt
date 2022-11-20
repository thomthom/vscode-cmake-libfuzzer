# Allow short runs during automated testing to see if something new breaks
set(FUZZ_RUNTIME 10
  CACHE STRING "Number of seconds to run fuzz tests during ctest run"
) # Default of 10 seconds

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(WARNING "Cannot enable fuzzing. Unsupported compiler: ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_ID})")
  return()
endif()
# TODO: Add Clang version check? As with older Clang version on Windows you had
# to manually find and link against the fuzzer libs.

# Interface for linking to Clangs libraries.
add_library(clang_fuzzer INTERFACE)

# https://llvm.org/docs/LibFuzzer.html#fuzzer-usage

# https://github.com/llvm/llvm-project/issues/56300
target_compile_definitions(clang_fuzzer INTERFACE
  _DISABLE_STRING_ANNOTATION
  _DISABLE_VECTOR_ANNOTATION
)

target_link_libraries(clang_fuzzer INTERFACE
  -fsanitize=fuzzer,address
)
target_compile_options(clang_fuzzer INTERFACE
  -fsanitize=fuzzer,address
)

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

add_fuzzer(fuzz_test.cpp)
