#include <fstream>
#include <cstdint>

int main() {
  using namespace std;
  auto fp = ofstream("tjg2.bin", ios::binary|ios::out);
  if (!fp)
    return 1;
  for (int i = 0; i < 256; ++i)
    fp.put((uint8_t) i);
  fp.close();
  return 0;
}
