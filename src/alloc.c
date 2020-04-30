#include "alloc.h"

/**
 * @brief Returns 1 if is end dir
 *
 * @param directory
 * @return int
 */
int32_t isEndDirectory(Directory* directory) {
  if (directory->file_type == EXT2_FT_UNKNOWN) {
    return 1;
  }

  return 0;
}

/**
 * @brief Allocates a block
 *
 * @param disk_info
 * @return int32_t
 */
int32_t allocateBlock(DiskInfo* disk_info) {
  GroupDesc group_desc;
  int8_t    buffer[disk_info->block_size];

  for (int32_t group = 0; group < disk_info->group_count; group++) {
    ioGroupDescriptor(disk_info, &group_desc, group, IOMODE_READ);
    ioBlock(disk_info, group_desc.bg_block_bitmap, (int8_t*)&buffer, IOMODE_READ);

    for (int32_t pos = 0; pos < disk_info->blocks_per_group / 8; pos++) {
      int32_t real_block_pos = (pos * 8) + (group * disk_info->blocks_per_group);

      if (real_block_pos > disk_info->block_count) {
        break;
      }

      int32_t bit = findFreeBit(buffer[pos], 0);

      if (bit != -1) {
        int32_t free_block_pos = 1 + bit + (pos * 8) + (group * disk_info->blocks_per_group);

        // Mark bitmap as used, thus alloc'ing it
        buffer[pos] |= (1 << bit);

        // Just write the entire block back to the disk
        ioBlock(disk_info, group_desc.bg_block_bitmap, (int8_t*)&buffer, IOMODE_WRITE);

        printf("alloc: allocateBlock(): info: Alloc'd block %d\n", free_block_pos);

        return free_block_pos;
      }
    }
  }

  printf("alloc: allocateBlock(): error: Failed to alloc block\n");
  exit(EXIT_FAILURE);

  return -1;
}

/**
 * @brief Deallocates a block
 *
 * @param disk_info
 * @param block_no
 * @return int32_t
 */
void deallocateBlock(DiskInfo* disk_info, int32_t block_no) {
  GroupDesc group_desc;
  int8_t    buffer[disk_info->block_size];

  int32_t group = block_no / disk_info->blocks_per_group;
  int32_t pos   = (block_no % disk_info->blocks_per_group) / 8;
  int8_t  bit   = block_no % 8;

  if (block_no == 0) {
    return;
  }

  // Dump 0's to the block we're deallocing
  bzero(buffer, disk_info->block_size);
  ioBlock(disk_info, block_no, (int8_t*)&buffer, IOMODE_WRITE);

  // Read the group desc and find the right block
  ioGroupDescriptor(disk_info, &group_desc, group, IOMODE_READ);
  ioBlock(disk_info, group_desc.bg_block_bitmap, (int8_t*)&buffer, IOMODE_READ);

  // Flip the offending bit
  buffer[pos] &= ~(1 << (bit - 1));

  // Dump the bitmap back down to the disk
  ioBlock(disk_info, group_desc.bg_block_bitmap, (int8_t*)&buffer, IOMODE_WRITE);
}

/**
 * @brief Appends a dir table with a new entry
 *
 * @param disk_info
 * @param inode_start
 * @param directory
 */
void allocateDirectoryEntry(DiskInfo* disk_info, int32_t inode_no, Directory* directory) {
  INode     root_inode;
  Directory root_dir;
  int64_t   read_index  = 0;
  int64_t   write_index = 0;
  time_t    now         = time(NULL);

  ioINode(disk_info, &root_inode, inode_no, IOMODE_READ);

  while (1) {
    // Read index must be a multiple of 4 to start a directory entry
    if (read_index % 4 != 0) {
      read_index += 4 - (read_index % 4);
    }

    read_index += ioDirectoryEntry(disk_info, &root_dir, &root_inode, read_index, IOMODE_READ);

    if (isEndDirectory(&root_dir)) {
      // We've found the end dir! Now we overrwrite it with the new dir:

      // Jump to the start of the end dir:
      write_index = read_index - 8 - root_dir.name_len;
      if (write_index % 4 != 0) {
        write_index -= 4 - (write_index % 4);
      }

      // Write the new directory entry to the disk:
      write_index += ioDirectoryEntry(disk_info, directory, &root_inode, write_index, IOMODE_WRITE);
      // printDirectory(disk_info, directory);

      // Now write the end dir back:
      write_index += ioDirectoryEntry(disk_info, &root_dir, &root_inode, write_index, IOMODE_WRITE);

      // Now increase INode link count (The root dir now links to it)
      root_inode.i_links_count++;
      root_inode.i_atime = now;
      // And write the INode back to disk:
      ioINode(disk_info, &root_inode, inode_no, IOMODE_WRITE);
      return;
    }
  }
}

/**
 * @brief Deallocatess a directory entry
 *
 * @param disk_info
 * @param inode_no
 * @param directory
 */
void deallocateDirectoryEntry(DiskInfo* disk_info, int32_t inode_no, char* to_remove_name) {
  INode     root_inode;
  Directory current_dir;

  // ptrs on fs
  int32_t read_index  = 0;
  int32_t write_index = 0;

  ioINode(disk_info, &root_inode, inode_no, IOMODE_READ);

  // Scan for the dir we want
  while (1) {
    if (read_index % 4 != 0) {
      read_index += 4 - (read_index % 4);
    }

    read_index += ioDirectoryEntry(disk_info, &current_dir, &root_inode, read_index, IOMODE_READ);

    if (strcmp(to_remove_name, current_dir.name) == 0) {
      // We found our guy to remove!
      write_index = read_index - 8 - current_dir.name_len;
      break;
    }
  }

  // Move all the other dirs up
  while (1) {
    if (read_index % 4 != 0) {
      read_index += 4 - (read_index % 4);
    }

    if (write_index % 4 != 0) {
      write_index += 4 - (write_index % 4);
    }

    read_index += ioDirectoryEntry(disk_info, &current_dir, &root_inode, read_index, IOMODE_READ);
    write_index +=
      ioDirectoryEntry(disk_info, &current_dir, &root_inode, write_index, IOMODE_WRITE);

    if (isEndDirectory(&current_dir)) {
      bzero(&current_dir, sizeof(Directory));
      ioDirectoryEntry(disk_info, &current_dir, &root_inode, write_index, IOMODE_WRITE);
      return;
    }
  }
}

/**
 * @brief Returns the INode no of a newly allocated INode
 * NOTE: INode number from the bitmap starts counting at 1
 *
 * @param disk_info
 * @param ext_info
 * @return int32_t
 */
int32_t allocateINode(State* state) {
  GroupDesc group_desc;
  int8_t    buffer[state->disk_info->block_size];

  for (int32_t group = 0; group < state->disk_info->group_count; group++) {
    ioGroupDescriptor(state->disk_info, &group_desc, group, IOMODE_READ);
    ioBlock(state->disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer, IOMODE_READ);

    for (int32_t pos = 0; pos < state->disk_info->inodes_per_group / 8; pos++) {
      int32_t bit = findFreeBit(buffer[pos], 0);

      if (bit != -1) {
        int32_t free_inode_pos = 1 + bit + (pos * 8) + (group * state->disk_info->inodes_per_group);

        // Mark bitmap as used, thus alloc'ing it
        buffer[pos] |= (1 << (bit));

        // Just write the entire block back to the disk
        ioBlock(state->disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer, IOMODE_WRITE);

        int16_t current_time = time(NULL);
        // Prep the INode with our standard data so we don't have to deal with it later:
        INode inode = { getDefaultMode(EXT2_FT_REG_FILE),
                        state->user.user_id,
                        0,
                        current_time,
                        current_time,
                        current_time,
                        0,
                        state->user.group_id,
                        0,
                        0,
                        0 };

        ioINode(state->disk_info, &inode, free_inode_pos, IOMODE_WRITE);

        // Now we know which INode we have
        return free_inode_pos;
      }
    }
  }

  return -1;
}

/**
 * @brief Allocate a number of blocks for an INode
 *
 * @param disk_info
 * @param inode
 * @param blocks_count
 */
void allocateINodeBlocks(DiskInfo* disk_info, INode* inode, int64_t blocks_count) {
  IndirectRange range = calculateIndirectRange(disk_info);
  for (int64_t block_pos = 0; block_pos < blocks_count; block_pos++) {
    int32_t block_no = 0;

    if (block_pos >= range.triple_start) {
      int32_t block_index =
        (block_pos - range.triple_start) / (range.indirects_per_block) %
        (range.indirects_per_block * range.indirects_per_block * range.indirects_per_block);
      int32_t block_offset = sizeof(int32_t) * block_index;

      if (inode->i_block[EXT2_INDIRECT_TRIPLE] == 0) {
        inode->i_block[EXT2_INDIRECT_TRIPLE] = allocateBlock(disk_info);
      }

      ioBlockPart(disk_info, (int8_t*)&block_no, inode->i_block[EXT2_INDIRECT_TRIPLE],
                  sizeof(int32_t), block_offset, IOMODE_READ);

      if (block_no == 0) {
        block_no = allocateBlock(disk_info);
        ioBlockPart(disk_info, (int8_t*)&block_no, inode->i_block[EXT2_INDIRECT_TRIPLE],
                    sizeof(int32_t), block_offset, IOMODE_WRITE);
      }
    }

    // Double Indirect reader:
    if (block_pos >= range.double_start) {
      int32_t block_index = (block_pos - range.double_start) / (range.indirects_per_block) %
                            (range.indirects_per_block * range.indirects_per_block);
      int32_t block_offset = sizeof(int32_t) * block_index;

      if (inode->i_block[EXT2_INDIRECT_DOUBLE] == 0) {
        inode->i_block[EXT2_INDIRECT_DOUBLE] = allocateBlock(disk_info);
      }

      // If triple indirect was NOT called, then work off of double indirect table
      if (!(block_pos >= range.triple_start)) {
        block_no = inode->i_block[EXT2_INDIRECT_DOUBLE];
      }

      ioBlockPart(disk_info, (int8_t*)&block_no, block_no, sizeof(int32_t), block_offset,
                  IOMODE_READ);

      if (block_no == 0) {
        block_no = allocateBlock(disk_info);
        ioBlockPart(disk_info, (int8_t*)&block_no, block_no, sizeof(int32_t), block_offset,
                    IOMODE_WRITE);
      }
    }

    // Single Indirect reader:
    if (block_pos >= range.single_start) {
      int32_t block_index  = (block_pos - range.single_start) % range.indirects_per_block;
      int32_t block_offset = sizeof(int32_t) * block_index;

      if (inode->i_block[EXT2_INDIRECT_SINGLE] == 0) {
        inode->i_block[EXT2_INDIRECT_SINGLE] = allocateBlock(disk_info);
      }

      // If double indirect was NOT called, then work off of single indirect table
      if (!(block_pos >= range.double_start)) {
        block_no = inode->i_block[EXT2_INDIRECT_SINGLE];
      }

      ioBlockPart(disk_info, (int8_t*)&block_no, block_no, sizeof(int32_t), block_offset,
                  IOMODE_READ);

      if (block_no == 0) {
        block_no = allocateBlock(disk_info);
        ioBlockPart(disk_info, (int8_t*)&block_no, block_no, sizeof(int32_t), block_offset,
                    IOMODE_WRITE);
      }
    }

    // Direct Block reader:
    if (block_pos < range.single_start) {
      block_no = inode->i_block[block_pos];

      if (block_no == 0) {
        block_no                  = allocateBlock(disk_info);
        inode->i_block[block_pos] = block_no;
      }
    }
  }

  inode->i_blocks = blocks_count;
}

/**
 * @brief Gets rid of a double indirect block
 *
 * @param disk_info
 * @param block_no
 */
void deallocateDoubleIndirectBlock(DiskInfo* disk_info, int64_t block_no) {
  int32_t buffer[disk_info->block_size / sizeof(int32_t)];

  if (block_no == 0) {
    return;
  }

  ioBlock(disk_info, block_no, (int8_t*)&buffer, IOMODE_READ);

  for (int32_t pos = 0; pos < disk_info->block_size / sizeof(int32_t); pos++) {
    if (buffer[pos] != 0) {
      deallocateBlock(disk_info, buffer[pos]);
    }
  }

  deallocateBlock(disk_info, block_no);
}

/**
 * @brief Gets rid of a triple indirect block
 *
 * @param disk_info
 * @param block_no
 */
void deallocateTripleIndirectBlock(DiskInfo* disk_info, int64_t block_no) {
  int32_t buffer[disk_info->block_size / sizeof(int32_t)];

  if (block_no == 0) {
    return;
  }

  ioBlock(disk_info, block_no, (int8_t*)&buffer, IOMODE_READ);

  for (int32_t pos = 0; pos < disk_info->block_size / sizeof(int32_t); pos++) {
    if (buffer[pos] != 0) {
      deallocateDoubleIndirectBlock(disk_info, buffer[pos]);
    }
  }

  deallocateBlock(disk_info, block_no);
}

/**
 * @brief Deallocates an INode
 *
 * @param disk_info
 * @param block_no
 * @return int32_t
 */
void deallocateINode(DiskInfo* disk_info, int32_t inode_no) {
  GroupDesc group_desc;
  INode     inode;
  int8_t    buffer[disk_info->block_size];

  int32_t group_no = inode_no / disk_info->inodes_per_group;
  int32_t pos      = inode_no % (disk_info->inodes_per_group / 8);
  int8_t  bit      = pos % 8;

  ioINode(disk_info, &inode, inode_no, IOMODE_READ);

  IndirectRange range = calculateIndirectRange(disk_info);

  // Kill all of the data blocks
  for (int64_t block_pos = 0; block_pos < inode.i_blocks; block_pos++) {
    int32_t block_no = 0;
    ioFileBlockHelper(disk_info, &block_no, &inode, &range, block_pos);

    deallocateBlock(disk_info, block_no);
  }

  // Kill the indirect blocks if they exist
  deallocateBlock(disk_info, inode.i_block[EXT2_INDIRECT_SINGLE]);
  deallocateDoubleIndirectBlock(disk_info, inode.i_block[EXT2_INDIRECT_DOUBLE]);
  deallocateTripleIndirectBlock(disk_info, inode.i_block[EXT2_INDIRECT_TRIPLE]);

  // Dump 0's to the block we're deallocing
  bzero(&inode, sizeof(INode));
  ioINode(disk_info, &inode, inode_no, IOMODE_WRITE);

  // Read the group desc and find the right block
  ioGroupDescriptor(disk_info, &group_desc, group_no, IOMODE_READ);
  ioBlock(disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer, IOMODE_READ);

  // Flip the offending bit
  buffer[pos] &= ~(1 << (bit - 1));

  // Dump the bitmap back down to the disk
  ioBlock(disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer, IOMODE_WRITE);
}

/**
 * @brief Allocs a dir table. new_dir must be populated prior to calling
 *
 * @param state
 * @param parent_dir
 * @param new_dir
 */
void allocateDirectoryTable(State* state, Directory* parent_dir, Directory* new_dir) {
  int32_t inode_no = allocateINode(state);
  int32_t block_no = allocateBlock(state->disk_info);

  // Create the INode and dump it to the disk
  INode inode;
  ioINode(state->disk_info, &inode, inode_no, IOMODE_READ);
  // bzero(&inode, sizeof(INode));

  inode.i_mode |= EXT2_S_IFDIR;
  inode.i_mode |= getDefaultMode(EXT2_FT_DIR);
  inode.i_block[0]    = block_no;
  inode.i_blocks      = 1;
  inode.i_size        = state->disk_info->block_size;
  inode.i_links_count = 1;

  inode.i_atime = time(NULL);
  inode.i_ctime = time(NULL);
  inode.i_mtime = time(NULL);

  ioINode(state->disk_info, &inode, inode_no, IOMODE_WRITE);

  new_dir->inode = inode_no;

  // Add . .. and end dirs
  Directory dirs[] = { { inode_no, 0, 1, EXT2_FT_DIR, "." },
                       { parent_dir->inode, 0, 2, EXT2_FT_DIR, ".." },
                       { 0, 0, 0, EXT2_FT_UNKNOWN, "\0" } };

  for (int32_t pos = 0, offset = 0; pos < sizeof(dirs) / sizeof(Directory); pos++) {
    if (offset % 4 != 0) {
      offset += 4 - (offset % 4);
    }

    offset += ioDirectoryEntry(state->disk_info, &dirs[pos], &inode, offset, IOMODE_WRITE);
  }
}