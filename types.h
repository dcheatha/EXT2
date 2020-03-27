#ifndef TYPES_H
#define TYPES_H

#include <ext2fs/ext2_fs.h>

/**
 * @brief Constant offset of the superblock
 */
const int32_t SUPERBLOCK_OFFSET = 1024;

/**
 * @brief ID of the root INode
 */
const int32_t INODE_ROOT_ID = 2;

typedef struct ext2_inode      INode;
typedef struct ext2_group_desc GroupDesc;

/**
 * @brief Keeps track of disk infomation
 */
typedef struct disk_info {
  int32_t file_desc;
  int64_t block_size;
  int32_t inode_table_offset;
} DiskInfo;

/**
 * @brief Struct to hold ext2 info
 */
typedef struct ext2_info {
  struct ext2_super_block super_block;
  struct ext2_group_desc  group_desc;
  struct ext2_inode       root;
} ExtInfo;

#endif