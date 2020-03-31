#ifndef TYPES_H
#define TYPES_H

#include <ext2fs/ext2_fs.h>

/**
 * @brief Constant offset of the superblock
 */
const int32_t SUPERBLOCK_OFFSET;

/**
 * @brief ID of the root INode
 */
const int32_t INODE_ROOT_ID;

typedef struct ext2_inode          INode;
typedef struct ext2_group_desc     GroupDesc;
typedef struct ext2_dir_entry_2    Directory;
typedef struct ext2_dir_entry_tail DirectoryTail;

/**
 * @brief Keeps track of disk infomation
 */
typedef struct disk_info {
  int32_t file_desc;
  int64_t block_size;
  int32_t inodes_per_group;
  int32_t blocks_per_group;
  int32_t group_count;
} DiskInfo;

/**
 * @brief Struct to hold ext2 info
 */
typedef struct ext2_info {
  struct ext2_super_block super_block;
  struct ext2_inode       root;
  int32_t                 first_inode_table_block;
} ExtInfo;

#endif