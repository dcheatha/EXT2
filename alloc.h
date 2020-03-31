#ifndef ALLOC_H
#define ALLOC_H

#include "io.h"
#include "types.h"

#include <time.h>

/**
 * @brief Allocates a new directory entry
 *
 * @param disk_info
 * @param inode_start
 * @param directory
 */
void allocateDirectoryEntry(DiskInfo* disk_info, int32_t inode_start, Directory* directory);

/**
 * @brief Returns 1 if is end dir
 *
 * @param directory
 * @return int
 */
int isEndDirectory(Directory* directory);

#endif