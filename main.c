#include "cksum.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

bool cksum_debug = true;

int main(int argc, const char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: cksum file...\n");
    return EXIT_FAILURE;
  }
  if (!strcmp(argv[1], "--version")) {
    printf("cksum (coreutils-9.7)\n");
    return EXIT_SUCCESS;
  }
  for (int i = 1; i != argc; ++i) {
    FILE* fp = fopen(argv[i], "rb");
    if (!fp) {
      perror(argv[i]);
      continue;
    }
    unsigned int crc_result = 0;
    uintmax_t file_length = 0;
    bool status = crc_sum_stream(fp, &crc_result, &file_length);
    if (!status) {
      printf("%s: cksum_fp failed\n", argv[i]);
      return EXIT_FAILURE;
    }

    printf("%u %ju %s\n", crc_result, file_length, argv[i]);
    fclose(fp);
  }
  return EXIT_SUCCESS;
} // main
