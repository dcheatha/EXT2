#include "utility.h"

/**
 * @brief Reads bytes from the disk
 *
 * @param file_desc file desc
 * @param block Block number
 * @param buffer
 * @param bytes Number of bytes to read
 */
void readBytes(DiskInfo* disk_info, int32_t offset, int8_t* buffer, int64_t bytes) {
  lseek(disk_info->file_desc, offset, SEEK_SET);
  read(disk_info->file_desc, buffer, bytes);
}

/**
 * @brief Reads a block from the disk
 *
 * @param file_desc file desc
 * @param block Block number
 * @param buffer
 */
void readBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer) {
  readBytes(disk_info, block, buffer, disk_info->block_size);
}

/**
 * @brief Reads an iNode from the disk.
 *
 * @param disk_info
 * @param i_node
 */
void readINode(DiskInfo* disk_info, int32_t number, INode* i_node) {
  readBytes(disk_info, disk_info->inode_table_offset + number * sizeof(INode), (int8_t*)i_node,
            sizeof(INode));
}

/**
 * @brief Loads common filesystem infomation and prepares data structures
 *
 * @param disk_info
 * @param ext_info
 */
void initializeFilesystem(DiskInfo* disk_info, ExtInfo* ext_info) {
  // The superblock is located at an offset of 1024 bytes, ie, the first block.
  readBytes(disk_info, SUPERBLOCK_OFFSET, (int8_t*)&ext_info->super_block,
            sizeof(struct ext2_super_block));

  if (ext_info->super_block.s_magic != EXT2_SUPER_MAGIC) {
    printf("Magical error with s_magic=%x\n", ext_info->super_block.s_magic);
    exit(EXIT_FAILURE);
  }

  // Store block size (needs to be 32bits to the left...)
  disk_info->block_size = 1024 << ext_info->super_block.s_log_block_size;

  // The group descriptor is the block right after the superblock, so read that in. Depending on the
  // block size, it could be in the 2nd or 3rd block.
  if (disk_info->block_size <= SUPERBLOCK_OFFSET) {
    // Read the 3rd block
    readBytes(disk_info, disk_info->block_size * 2, (int8_t*)&ext_info->super_block,
              sizeof(struct ext2_group_desc));
  } else {
    // Read the 2nd block
    readBytes(disk_info, disk_info->block_size, (int8_t*)&ext_info->super_block,
              sizeof(struct ext2_group_desc));
  }

  disk_info->inode_table_offset = ext_info->group_desc.bg_inode_table;

  // ext_info.root = ext_info.group_desc.bg_inode_bitmap;
}
