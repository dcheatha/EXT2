#include "utility.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Simulates the EXT2 Filesystem
 *
 * @param argc
 * @param argv
 */
int32_t main(int32_t argc, char** argv) {
  ExtInfo  ext_info;
  DiskInfo disk_info;

  if (argc < 1) {
    printf("Please provide a disk image\n");
    return EXIT_FAILURE;
  }

  disk_info.file_desc = open(*(argv + 1), O_RDWR);

  if (disk_info.file_desc == 0) {
    printf("Unable to open file=%s\n", *(argv + 1));
    return EXIT_FAILURE;
  }

  initializeFilesystem(&disk_info, &ext_info);

  return EXIT_SUCCESS;
}