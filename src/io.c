#include "io.h"

/**
 * @brief Perform an IO operation on some bytes
 *
 * @param disk_info
 * @param buffer
 * @param length Length of buffer
 * @param offset Offset on disk
 * @param mode
 */
void ioBytes(DiskInfo* disk_info, int8_t* buffer, int64_t length, int64_t offset, IOMode mode) {
  lseek(disk_info->file_desc, offset, SEEK_SET);

  switch (mode) {
    case IOMODE_READ: {
      read(disk_info->file_desc, buffer, length);
      return;
    }
    case IOMODE_WRITE: {
      write(disk_info->file_desc, buffer, length);
      return;
    }
    default: {
      printf("io: ioBytes(): error: Unsupported IOMode %d\n", mode);
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * @brief Do an IO operation on a block
 *
 * @param disk_info
 * @param buffer
 * @param block
 * @param mode
 */
void ioBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer, IOMode mode) {
  ioBytes(disk_info, buffer, block * disk_info->block_size, disk_info->block_size, mode);
}

/**
 * @brief Do an IO operation on part of a block
 *
 * @param disk_info
 * @param block
 * @param buffer
 * @param bytes
 * @param offset
 * @param mode
 */
void ioBlockPart(DiskInfo* disk_info, int8_t* buffer, int64_t block, int64_t length, int64_t offset,
                 IOMode mode) {
  if (length + offset > disk_info->block_size) {
    printf(
      "io: ioBlockPart(): error: Attempted to seek past block boundaries on block %ld from bytes "
      "%ld to %ld "
      "\n",
      block, offset, offset + length);
    exit(EXIT_FAILURE);
  }

  ioBytes(disk_info, buffer, block * disk_info->block_size + offset, length, mode);
}

/**
 * @brief Do an IO operation on a group descriptor
 *
 * @param disk_info
 * @param group
 * @param group_no
 * @param mode
 */
void ioGroupDescriptor(DiskInfo* disk_info, GroupDesc* group, int64_t group_no, IOMode mode) {
  ioBlockPart(disk_info, (int8_t*)group, 2, sizeof(GroupDesc), group_no * sizeof(GroupDesc), mode);
}

/**
 * @brief Do an IO operation on an INode
 *
 * @param disk_info
 * @param inode
 * @param inode_no
 * @param mode
 */
void ioINode(DiskInfo* disk_info, INode* inode, int64_t inode_no, IOMode mode) {
  int64_t group_no    = (inode_no - 1) / disk_info->inodes_per_group;
  int32_t table_index = (inode_no - 1) % disk_info->inodes_per_group;

  GroupDesc group_desc;

  ioGroupDescriptor(disk_info, &group_desc, group_no, IOMODE_READ);

  ioBytes(disk_info, (int8_t*)inode, sizeof(INode),
          group_desc.bg_inode_table * disk_info->block_size + (table_index * sizeof(INode)), mode);

  // if read   inode->i_blocks = inode->i_blocks / (2 << disk_info->s_log_block_size);
}

/**
 * @brief Does an IO operation on a Directory Entry
 *
 * @param disk_info
 * @param directory
 * @param inode
 * @param offset
 * @param mode
 * @return int64_t
 */
int64_t ioDirectoryEntry(DiskInfo* disk_info, Directory* directory, INode* inode, int64_t offset,
                         IOMode mode) {
  int64_t block_index     = offset / disk_info->block_size;
  int64_t directory_index = offset % disk_info->block_size;
  int64_t padding_length  = 0;

  directory->rec_len = 8 + directory->name_len;

  // Do the first part of the struct
  ioFile(disk_info, (int8_t*)directory, inode, 8, directory_index, mode);

  // Do the name
  ioFile(disk_info, (int8_t*)directory->name, inode, directory->name_len, directory_index + 8,
         mode);

  // If we are doing a write, then we should add some padding:
  // However, we need to control for the edge case where we are at the very start of a new block,
  // but that may be done later:
  if (directory_index % 4 != 0 && mode == IOMODE_WRITE) {
    int8_t padding_data[] = { 0, 0, 0, 0 };
    padding_length        = 4 - (directory_index % 4);

    ioFile(disk_info, (int8_t*)&padding_data, inode, padding_length,
           directory_index + 8 + directory->name_len, IOMODE_WRITE);
  }

  return 8 + directory->name_len + padding_length;
}

/**
 * @brief Do an IO operation on a file (the data an INode points to...)
 *
 * @param disk_info
 * @param buffer Buffer to store data
 * @param inode File to IO
 * @param length
 * @param offset
 * @param mode
 */
void ioFile(DiskInfo* disk_info, int8_t* buffer, INode* inode, int64_t length, int64_t offset,
            IOMode mode) {
  int32_t blocks_to_io  = length / disk_info->block_size + (length % disk_info->block_size != 0);
  int32_t offset_blocks = offset / disk_info->block_size;

  bzero(buffer, length);

  printf("io: ioFile(): info: Seeking from %d to %d\n", offset, offset + length);

  if (blocks_to_io > inode->i_blocks) {
    printf("io: ioFile(): error: Requested to seek %d blocks when there are only %d blocks\n",
           blocks_to_io, inode->i_blocks);

    exit(EXIT_FAILURE);
  }

  if (blocks_to_io + offset_blocks > inode->i_blocks) {
    printf("io: ioFile(): warn: Requested to seek blocks %d to %d when there are only %d blocks\n",
           offset_blocks, blocks_to_io + offset_blocks, inode->i_blocks);

    exit(EXIT_FAILURE);
  }

  IndirectRange range      = calculateIndirectRange(disk_info);
  int64_t       buffer_pos = 0;

  for (int64_t block_pos = offset_blocks;
       block_pos < blocks_to_io + offset_blocks && block_pos < inode->i_blocks; block_pos++) {
    int32_t io_length = disk_info->block_size;  // Bytes to seek from this block
    int32_t io_offset = 0;                      // Offset in bytes to seek from this block
    int32_t block_no  = 0;                      // Block to read

    // If we are the first read, adjust for our offset:
    if (block_pos == offset_blocks) {
      io_length -= offset;
      io_offset += offset;
    }

    // If we are the last read, adjust for our offset:
    if (block_pos == blocks_to_io + offset_blocks) {
      io_length -= offset;
    }

    // And make sure we don't write past buffer:
    if (buffer_pos + io_length > length) {
      io_length = length - buffer_pos;
    }

    // Business logic starts here:
    // TL;DR for Biz logic: We continuous adjust block_no to point to the next block we need to read
    // from. For example, to read a double indirect block block_no is set to the double indirect
    // block. We read the correct index, and block_no now points to a single indirect block. We read
    // the index again, and now block_no points to the correct data. If I have more time, I will
    // cache the blocks.

    // Triple Indirect reader:
    if (block_pos >= range.triple_start) {
      int32_t block_index =
        (block_pos - range.triple_start) / (range.indirects_per_block) %
        (range.indirects_per_block * range.indirects_per_block * range.indirects_per_block);
      int32_t block_offset = sizeof(int32_t) * block_index;

      ioBlockPart(disk_info, (int8_t*)&block_no, inode->i_block[EXT2_INDIRECT_TRIPLE],
                  sizeof(int32_t), block_offset, IOMODE_READ);
    }

    // Double Indirect reader:
    if (block_pos >= range.double_start) {
      int32_t block_index = (block_pos - range.double_start) / (range.indirects_per_block) %
                            (range.indirects_per_block * range.indirects_per_block);
      int32_t block_offset = sizeof(int32_t) * block_index;

      // If triple indirect was NOT called, then work off of double indirect table
      if (!(block_pos >= range.triple_start)) {
        block_no = inode->i_block[EXT2_INDIRECT_DOUBLE];
      }

      ioBlockPart(disk_info, (int8_t*)&block_no, block_no, sizeof(int32_t), block_offset,
                  IOMODE_READ);
    }

    // Single Indirect reader:
    if (block_pos >= range.single_start) {
      int32_t block_index  = (block_pos - range.single_start) % range.indirects_per_block;
      int32_t block_offset = sizeof(int32_t) * block_index;

      // If double indirect was NOT called, then work off of single indirect table
      if (!(block_pos >= range.double_start)) {
        block_no = inode->i_block[EXT2_INDIRECT_SINGLE];
      }

      ioBlockPart(disk_info, (int8_t*)&block_no, block_no, sizeof(int32_t), block_offset,
                  IOMODE_READ);
    }

    // Direct Block reader:
    if (block_pos < range.single_start) {
      block_no = inode->i_block[block_pos];
    }

    ioBlockPart(disk_info, buffer + buffer_pos, block_no, io_length, io_offset, mode);
    buffer_pos += io_length;

    if (block_pos > range.triple_end) {
      printf("io: ioFile(): error: Requested block beyond max supported range of EXT2\n");
      exit(EXIT_FAILURE);
    }
  }
}