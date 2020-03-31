#ifndef UTILITY_H
#define UTILITY_H

#include "io.h"
#include "print.h"
#include "types.h"

#include <stdlib.h>

/**
 * @brief Reads and prepares data structures for Filesystem
 *
 * @param disk_info
 * @param ext_info
 */
void initializeFilesystem(DiskInfo* disk_info, ExtInfo* ext_info);

#endif