#ifndef IO_H
#define IO_H

#include "types.h"
#include "utility.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum IOMode { IOMODE_READ, IOMODE_WRITE } typedef IOMode;

/**
 * @brief Keeps track of the indirect block names
 */
#define EXT2_INDIRECT_SINGLE 12
#define EXT2_INDIRECT_DOUBLE 13
#define EXT2_INDIRECT_TRIPLE 14

/**
 * @brief Does an IO operation on a sequence of bytes on the disk
 *
 * @param disk_info
 * @param buffer
 * @param length
 * @param offset
 * @param mode
 */
void ioBytes(DiskInfo* disk_info, int8_t* buffer, int64_t length, int64_t offset, IOMode mode);

/**
 * @brief Does an IO operation on a block
 *
 * @param disk_info
 * @param block
 * @param buffer
 * @param mode
 */
void ioBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer, IOMode mode);

/**
 * @brief Does an IO operation on a portion of a block
 *
 * @param disk_info
 * @param buffer
 * @param block
 * @param length
 * @param offset
 * @param mode
 */
void ioBlockPart(DiskInfo* disk_info, int8_t* buffer, int64_t block, int64_t length, int64_t offset,
                 IOMode mode);

/**
 * @brief Does an IO operation on a group descriptor
 *
 * @param disk_info
 * @param group
 * @param group_no
 * @param mode
 */
void ioGroupDescriptor(DiskInfo* disk_info, GroupDesc* group, int64_t group_no, IOMode mode);

/**
 * @brief Does an IO operation on an INode
 *
 * @param disk_info
 * @param inode
 * @param inode_no
 * @param mode
 */
void ioINode(DiskInfo* disk_info, INode* inode, int64_t inode_no, IOMode mode);

/**
 * @brief Does an IO operation on a directory entry
 *
 * @param disk_info
 * @param directory
 * @param inode
 * @param offset in bytes
 * @param mode
 * @return int64_t offset in bytes after seeking is done
 */
int64_t ioDirectoryEntry(DiskInfo* disk_info, Directory* directory, INode* inode, int64_t offset,
                         IOMode mode);

/**
 * @brief Reads data from an INode in order
 *
 * @param disk_info
 * @param block_no
 * @param inode
 * @param range
 * @param block_pos
 */
void ioFileBlockHelper(DiskInfo* disk_info, int32_t* block_no, INode* inode, IndirectRange* range,
                       int64_t block_pos);

/**
 * @brief Does an IO operation on a file (really, just the data in an INode)
 *
 * @param disk_info
 * @param buffer
 * @param inode
 * @param length
 * @param offset
 * @param mode
 */
void ioFile(DiskInfo* disk_info, int8_t* buffer, INode* inode, int64_t length, int64_t offset,
            IOMode mode);

#endif