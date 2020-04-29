#ifndef TYPES_H
#define TYPES_H

#include <ext2fs/ext2_fs.h>

/**
 * @brief Filesystem perm constants
 * I needed these constants, but I couldn't find the appropriate header for them. Please
 * excuse the copy / paste here.
 */
#define EXT2_S_IFMT 0xF000   /* format mask  */
#define EXT2_S_IFSOCK 0xC000 /* socket */
#define EXT2_S_IFLNK 0xA000  /* symbolic link */
#define EXT2_S_IFREG 0x8000  /* regular file */
#define EXT2_S_IFBLK 0x6000  /* block device */
#define EXT2_S_IFDIR 0x4000  /* directory */
#define EXT2_S_IFCHR 0x2000  /* character device */
#define EXT2_S_IFIFO 0x1000  /* fifo */
#define EXT2_S_ISUID 0x0800  /* SUID */
#define EXT2_S_ISGID 0x0400  /* SGID */
#define EXT2_S_ISVTX 0x0200  /* sticky bit */
#define EXT2_S_IRWXU 0x01C0  /* user access rights mask */
#define EXT2_S_IRUSR 0x0100  /* read */
#define EXT2_S_IWUSR 0x0080  /* write */
#define EXT2_S_IXUSR 0x0040  /* execute */
#define EXT2_S_IRWXG 0x0038  /* group access rights mask */
#define EXT2_S_IRGRP 0x0020  /* read */
#define EXT2_S_IWGRP 0x0010  /* write */
#define EXT2_S_IXGRP 0x0008  /* execute */
#define EXT2_S_IRWXO 0x0007  /* others access rights mask */
#define EXT2_S_IROTH 0x0004  /* read */
#define EXT2_S_IWOTH 0x0002  /* write */
#define EXT2_S_IXOTH 0x0001  /* execute */

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
  int32_t s_log_block_size;
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
enum Command {
  LS,
  MKDIR,
  RMDIR,
  CREATE,
  LINK,
  UNLINK,
  MKFS,
  CAT,
  CP,
  MENU,
  CD,
  DISKINFO,
  INODEINFO
} typedef Command;

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

/**
 * @brief
 */
typedef struct IndirectRange {
  int32_t indirects_per_block;
  int32_t single_start;
  int32_t double_start;
  int32_t triple_start;
  int32_t triple_end;
} IndirectRange;

#endif