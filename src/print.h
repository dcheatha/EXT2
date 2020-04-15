#ifndef PRINT_H
#define PRINT_H

#include "io.h"
#include "types.h"
#include "alloc.h"

#include <stdio.h>

/**
 * @brief Prints basic ext2 info
 *
 * @param ext_info
 * @param disk_info
 */
void printDiskInfomation(ExtInfo* ext_info, DiskInfo* disk_info);

/**
 * @brief Prints info for a single dir entry
 *
 * @param directory
 */
void printDirectory(Directory* directory);

/**
 * @brief Prints an entire dir table
 *
 * @param disk_info
 * @param inode_start
 */
void printDirectoryTable(DiskInfo* disk_info, int32_t inode_start);

#endif