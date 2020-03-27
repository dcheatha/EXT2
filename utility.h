#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief Reads Bytes from Filesystem
 *
 * @param disk_info
 * @param offset
 * @param buffer
 * @param bytes
 */
void readBytes(DiskInfo* disk_info, int32_t offset, int8_t* buffer, int64_t bytes);

/**
 * @brief Reads a block from Filesystem
 *
 * @param disk_info
 * @param block
 * @param buffer
 */
void readBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer);

/**
 * @brief Reads and prepares data structures for Filesystem
 *
 * @param disk_info
 * @param ext_info
 */
void initializeFilesystem(DiskInfo* disk_info, ExtInfo* ext_info);

#endif