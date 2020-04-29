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
 * @brief Reads bytes from the disk
 *
 * @param disk_info
 * @param offset
 * @param buffer
 * @param bytes
 */
void readBytes(DiskInfo* disk_info, int32_t offset, int8_t* buffer, int64_t bytes);

/**
 * @brief Writes bytes to the disk
 *
 * @param disk_info
 * @param offset
 * @param buffer
 * @param bytes
 */
void writeBytes(DiskInfo* disk_info, int32_t offset, int8_t* buffer, int64_t bytes);

/**
 * @brief Reads an entire block
 *
 * @param disk_info
 * @param block
 * @param buffer
 */
void readBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer);

/**
 * @brief Writes an entire block
 *
 * @param disk_info
 * @param block
 * @param buffer
 */
void writeBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer);

/**
 * @brief Reads part of a block
 *
 * @param disk_info
 * @param block
 * @param buffer
 * @param bytes
 * @param offset
 */
void readBlockBytes(DiskInfo* disk_info, int64_t block, int8_t* buffer, int64_t bytes,
                    int64_t offset);

/**
 * @brief Writes to part of a block
 *
 * @param disk_info
 * @param block
 * @param buffer
 * @param bytes
 * @param offset
 */
void writeBlockBytes(DiskInfo* disk_info, int64_t block, int8_t* buffer, int64_t bytes,
                     int64_t offset);

/**
 * @brief Reads a group desc for a group
 *
 * @param disk_info
 * @param group
 * @param group_desc
 */
void readGroupDesc(DiskInfo* disk_info, int64_t group, GroupDesc* group_desc);

/**
 * @brief Reads an INode from the disk
 *
 * @param disk_info
 * @param number
 * @param i_node
 */
void readINode(DiskInfo* disk_info, int32_t number, INode* i_node);

/**
 * @brief Writes an INode to the disk
 *
 * @param disk_info
 * @param number
 * @param i_node
 */
void writeINode(DiskInfo* disk_info, int32_t number, INode* i_node);

/**
 * @brief Reads from the blocks specified by an INode in order
 *
 * @param disk_info
 * @param inode
 * @param buffer
 * @param bytes
 */
void readINodeData(DiskInfo* disk_info, INode* inode, int8_t* buffer, int32_t bytes);

/**
 * @brief Reads a directory entry given an INode with dir data
 *
 * @param disk_info
 * @param inode
 * @param directory
 * @param offset
 * @return int
 */
int32_t readDirectory(DiskInfo* disk_info, INode* inode, Directory* directory, int32_t offset);

/**
 * @brief Writes a directory entry given an INode
 *
 * @param disk_info
 * @param inode
 * @param directory
 * @param offset
 * @return int
 */
int32_t writeDirectory(DiskInfo* disk_info, INode* inode, Directory* directory, int32_t offset);

/**
 * @brief Give this a path string and it returns the Dir for the file, if it exists
 *
 * @param state
 * @param parameter
 * @param found_file
 * @return int32_t
 */
int32_t readPath(State* state, char* parameter, Directory* found_file);

/**
 * @brief Like readPath, but returns the parent dir
 *
 * @param state
 * @param parameter
 * @param found_file
 * @return int32_t
 */
int32_t readPathParent(State* state, char* parameter, Directory* found_file);

/**
 * @brief Determines if a path exists
 *
 * @param state
 * @param parameter
 * @return int8_t
 */
int8_t readPathExists(State* state, char* parameter);

/**
 * @brief Reads a file
 *
 * @param disk_info
 * @param inode
 * @param buffer
 * @param bytes
 * @param offset
 */
void readFile(DiskInfo* disk_info, INode* inode, int8_t* buffer, int32_t bytes, int32_t offset);

/**
 * @brief Writes to a file
 *
 * @param disk_info
 * @param inode
 * @param buffer
 * @param bytes
 * @param offset
 */
void writeFile(DiskInfo* disk_info, INode* inode, int8_t* buffer, int32_t bytes, int32_t offset);

#endif