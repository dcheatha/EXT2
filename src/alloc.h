#ifndef ALLOC_H
#define ALLOC_H

#include "io.h"
#include "types.h"
#include "utility.h"

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
int32_t isEndDirectory(Directory* directory);

/**
 * @brief Allocates an INode
 *
 * @param state
 * @return int32_t
 */
int32_t allocateINode(State* state);

/**
 * @brief Allocates a block
 *
 * @param disk_info
 * @return int32_t
 */
int32_t allocateBlock(DiskInfo* disk_info);

/**
 * @brief Allocs a new dir table
 *
 * @param state
 * @param parent_dir
 * @param new_dir
 */
void allocateDirectoryTable(State* state, Directory* parent_dir, Directory* new_dir);

#endif