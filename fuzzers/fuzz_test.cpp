#include <iostream>
#include <stddef.h>
#include <stdint.h>

bool FuzzMe(const uint8_t *Data, size_t DataSize) {
  return DataSize >= 3 &&
      Data[0] == 'F' &&
      Data[1] == 'U' &&
      Data[2] == 'Z' &&
      Data[3] == 'Z';  // :‑<
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  // std::cout << "Hello Fuzzy...\n";
  FuzzMe(data, size);
  return 0;
}
