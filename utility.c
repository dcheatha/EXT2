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
  printf("(R bytes=%i offset=%i)\n", bytes, offset);
  lseek(disk_info->file_desc, offset, SEEK_SET);
  read(disk_info->file_desc, buffer, bytes);
}

/**
 * @brief Write bytes to the disk
 *
 * @param disk_info
 * @param offset
 * @param buffer
 * @param bytes
 */
void writeBytes(DiskInfo* disk_info, int32_t offset, int8_t* buffer, int64_t bytes) {
  printf("(W bytes=%i offset=%i)\n", bytes, offset);
  lseek(disk_info->file_desc, offset, SEEK_SET);
  write(disk_info->file_desc, buffer, bytes);
}

/**
 * @brief Reads a block from the disk
 *
 * @param file_desc file desc
 * @param block Block number
 * @param buffer
 */
void readBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer) {
  printf("[R block=%i] ", block);
  readBytes(disk_info, block * disk_info->block_size, buffer, disk_info->block_size);
}

/**
 * @brief Reads a block at an offset for x bytes
 *
 * @param disk_info
 * @param block Block no to read
 * @param buffer Buffer to read into
 * @param bytes Amount of bytes to read
 * @param offset Offset from start of block to read
 */
void readBlockBytes(DiskInfo* disk_info, int64_t block, int8_t* buffer, int64_t bytes,
                    int64_t offset) {
  printf("[R block=%i bytes=%i offset=%i] ", block, bytes, offset);
  readBytes(disk_info, block * disk_info->block_size + offset, buffer, bytes);
}

/**
 * @brief Determines if a group has a copy of the superblock
 *
 * @param group
 * @return int
 */
int groupHasSuperBlock(const int32_t group) {
  if (group == 0 || group == 1) {
    return 1;
  }

  switch (group % 10) {
    case 3:
    case 5:
    case 7: return 1;
  }

  return 0;
}

/**
 * @brief Reads a group desc into memory
 * The group desc seems to always be at the 2nd block of a group
 * @param disk_info
 * @param group Group to read from
 * @param group_desc Group desc to read into
 */
void readGroupDesc(DiskInfo* disk_info, int64_t group, GroupDesc* group_desc) {
  readBlockBytes(disk_info, 2, (int8_t*)group_desc, sizeof(GroupDesc), sizeof(GroupDesc) * group);
}

/**
 * @brief Reads an INode from the disk.
 *
 * block group = (inode - 1) / s_inodes_per_group
 * local inode index = (inode - 1) % s_inodes_per_group
 *
 * @param disk_info
 * @param i_node
 */
void readINode(DiskInfo* disk_info, int32_t number, INode* i_node) {
  GroupDesc group_desc;
  int32_t   block       = (number - 1) / disk_info->inodes_per_group;
  int32_t   table_index = (number - 1) % disk_info->inodes_per_group;

  readGroupDesc(disk_info, block, &group_desc);
  printf("{R INode=%i} ", number);
  readBlockBytes(disk_info, group_desc.bg_inode_table, (int8_t*)i_node, sizeof(INode),
                 table_index * sizeof(INode));
}

/**
 * @brief Just read all of the data from an INode
 *
 * @param disk_info
 * @param inode
 */
void readINodeData(DiskInfo* disk_info, INode* inode, int8_t* buffer, int32_t bytes) {
  int32_t block_index   = 0;
  int32_t buffer_offset = 0;

  // TODO: Support links
  for (int pos = 0; pos < 15; pos++) {
    if (inode->i_block[pos] == NULL) {
      continue;
    }

    int32_t bytes_to_read = (bytes - buffer_offset) > disk_info->block_size
                              ? disk_info->block_size
                              : (bytes - buffer_offset);
    readBlockBytes(disk_info, inode->i_block[pos], buffer + buffer_offset, bytes_to_read, 0);
    buffer_offset += bytes_to_read;
  }
}

/**
 * @brief Returns 1 if is end dir
 *
 * @param directory
 * @return int
 */
int isEndDirectory(Directory* directory) {
  if (directory->file_type == EXT2_FT_UNKNOWN) {
    return 1;
  }

  return 0;
}

/**
 * @brief Reads a directory given an INode
 *
 * @param disk_info
 * @param i_node
 * @param directory
 * @param offset
 */
int readDirectory(DiskInfo* disk_info, INode* inode, Directory* directory, int32_t offset) {
  int32_t block_index     = offset / disk_info->block_size;
  int32_t directory_index = offset % disk_info->block_size;

  printf("{R Directory offset=%i (block_index=%i directory_index=%i)} ", offset, block_index,
         directory_index);

  // Read the first bit of the struct into memory
  readBlockBytes(disk_info, inode->i_block[block_index], (int8_t*)directory, 8, directory_index);

  // Clean up the place to dump the string
  bzero(directory->name, sizeof(directory->name));

  // Read the name into the directory
  readBlockBytes(disk_info, inode->i_block[block_index], (int8_t*)directory->name,
                 directory->name_len, directory_index + 8);

  return 8 + directory->name_len;
}

/**
 * @brief Dumps infomation about the disk for debugging
 *
 * @param ext_info
 */
void printDiskInfomation(ExtInfo* ext_info, DiskInfo* disk_info) {
  printf("EX2 Filesystem loaded:\n");
  printf("%20s: %10s\n", "Volume Name", ext_info->super_block.s_volume_name);
  printf("%20s: %10s\n", "Last Mount Path", ext_info->super_block.s_last_mounted);

  printf("%20s: %10u\n", "Blocks per Group", ext_info->super_block.s_blocks_per_group);
  printf("%20s: %10i\n", "Blocks Count",
         (int64_t)ext_info->super_block.s_blocks_count << 32 |
           ext_info->super_block.s_blocks_count);
  printf("%20s: %10u\n", "Block Size", 1024 << ext_info->super_block.s_log_block_size);
  printf("%20s: %10u\n", "First Block", ext_info->super_block.s_first_data_block);
  printf("%20s: %10u\n", "Free Blocks", ext_info->super_block.s_free_blocks_count);
  printf("%20s: %10u\n", "INode Count", ext_info->super_block.s_inodes_count);
  printf("%20s: %10u\n", "First INode", ext_info->super_block.s_first_ino);
  printf("%20s: %10u\n", "Free INodes", ext_info->super_block.s_free_inodes_count);
  printf("%20s: %10u\n", "INodes per Group", ext_info->super_block.s_inodes_per_group);
}

void printDirectory(Directory* directory) {
  printf("%20s: %s\n", "Name", directory->name);
  printf("%20s: %10u\n", "Name Length", directory->name_len);
  printf("%20s: %10u\n", "INode", directory->inode);
  printf("%20s: %10u\n", "Rec Len", directory->rec_len);
  printf("%20s: %10u\n", "File Type", directory->file_type);
}

void printINode(INode* inode) {
  printf("%20s: %10i\n", "Size", inode->i_size);
  printf("%20s: %10i\n", "Blocks", inode->i_blocks);

  for (int pos = 0; pos < 15; pos++) {
    printf("%15s[%3i]: %10i\n", "Block", pos, inode->i_block[pos]);
  }

  printf("%20s: %10i\n", "Mode", inode->i_mode);
}

/**
 * @brief Loads common filesystem infomation and prepares data structures
 *
 * Layout of each block group:
 * | Super Block | Group Desc | Block Bitmap | INode Bitmap | INode Table | Data Blocks
 *
 * @param disk_info
 * @param ext_info
 */
void initializeFilesystem(DiskInfo* disk_info, ExtInfo* ext_info) {
  // The superblock is located at an offset of 1024 bytes, ie, the first block.
  readBytes(disk_info, SUPERBLOCK_OFFSET, (int8_t*)&ext_info->super_block,
            sizeof(struct ext2_super_block));

  if (ext_info->super_block.s_magic != EXT2_SUPER_MAGIC) {
    printf("Magical error with s_magic=%x (is this an EXT2 filesystem?)\n",
           ext_info->super_block.s_magic);
    exit(EXIT_FAILURE);
  }

  // Store block size (needs to be 32bits to the left...)
  disk_info->block_size = 1024 << ext_info->super_block.s_log_block_size;

  // Need to know how many blocks there are in a group
  disk_info->blocks_per_group = ext_info->super_block.s_blocks_per_group;

  // Need to know how many inodes there are in a group
  disk_info->inodes_per_group = ext_info->super_block.s_inodes_per_group;

  // Can't figure out how to get this number. Blocks Count returns the size of the filesystem
  // in megabytes which makes no sense whatsoever.
  disk_info->group_count = 2;

  // The group descriptor (for the first group) is the block right after the superblock, so read
  // that in. Depending on the block size, it could be in the 2nd or 3rd block.
  if (disk_info->block_size <= SUPERBLOCK_OFFSET) {
    // Read the 3rd block
    readBytes(disk_info, disk_info->block_size * 2, (int8_t*)&ext_info->super_block,
              sizeof(struct ext2_group_desc));
  } else {
    // Read the 2nd block
    readBytes(disk_info, disk_info->block_size, (int8_t*)&ext_info->super_block,
              sizeof(struct ext2_group_desc));
  }

  printDiskInfomation(ext_info, disk_info);

  INode     root_inode;
  Directory root_dir;
  int32_t   dir_index = 0;

  readINode(disk_info, EXT2_ROOT_INO, &root_inode);
  printf("Root INode infomation:\n");

  while (1) {
    if (dir_index % 4 != 0) {
      dir_index += 4 - (dir_index % 4);
    }

    dir_index += readDirectory(disk_info, &root_inode, &root_dir, dir_index);

    if (isEndDirectory(&root_dir)) {
      break;
    }

    printDirectory(&root_dir);

    if (root_dir.file_type == EXT2_FT_REG_FILE) {
      INode data_inode;
      readINode(disk_info, root_dir.inode, &data_inode);
      printINode(&data_inode);

      char buffer[1024] = { 0 };
      readINodeData(disk_info, &data_inode, (int8_t*)&buffer, 1024);
      printf("%20s: %s\n", "Data", buffer);
    }
  }
}
