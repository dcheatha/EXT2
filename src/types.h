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

/**
 * @brief Command IDs for interacting with the filesystem
 * MKFS = Make Filesystem
 */
enum Command { LS, MKDIR, RMDIR, CREATE, LINK, UNLINK, MKFS } typedef Command;

/**
 * @brief Keeps track of the current path on disk
 * NOTE: Must dealloc these
 */
struct Path {
  char         name[EXT2_NAME_LEN];
  int32_t      inode_number;
  struct Path* parent;
  struct Path* child;
} typedef Path;

/**
 * @brief Keeps track of user infomation
 */
struct User {
  int32_t user_id;
  int32_t group_id;
} typedef User;

/**
 * @brief Stores the current state of the filesystem we're emulating
 */
struct State {
  ExtInfo*  ext_info;
  DiskInfo* disk_info;
  User      user;
  Path*     path_root;
  Path*     path_cwd;
  Directory current_file;
} typedef State;

#endif