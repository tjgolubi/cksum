#include <fstream>
#include <cstdint>
#include <cstdlib>

int main() {
  using namespace std;
  auto fp = ofstream("tjg256.bin", ios::binary | ios::out);
  if (!fp)
    return EXIT_FAILURE;
  for (int i = 0; i < 256; ++i)
    fp.put((uint8_t) i);
  fp.close();
  return EXIT_SUCCESS;
} // main
