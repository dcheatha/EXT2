#include "io.h"

/**
 * @brief Reads bytes from the disk
 *
 * @param file_desc file desc
 * @param block Block number
 * @param buffer
 * @param bytes Number of bytes to read
 */
void readBytes(DiskInfo* disk_info, int32_t offset, int8_t* buffer, int64_t bytes) {
  printf("(R bytes bytes=%li offset=%i)\n", bytes, offset);
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
  printf("(W bytes bytes=%li offset=%i)\n", bytes, offset);
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
  printf("[R block block=%li] ", block);
  readBytes(disk_info, block * disk_info->block_size, buffer, disk_info->block_size);
}

/**
 * @brief Writes a block to the disk
 *
 * @param file_desc file desc
 * @param block Block number
 * @param buffer
 */
void writeBlock(DiskInfo* disk_info, int64_t block, int8_t* buffer) {
  printf("[W block block=%li] ", block);
  writeBytes(disk_info, block * disk_info->block_size, buffer, disk_info->block_size);
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
  printf("[R block block=%li bytes=%li offset=%li] ", block, bytes, offset);
  readBytes(disk_info, block * disk_info->block_size + offset, buffer, bytes);
}

/**
 * @brief Writes to a block at an offset for x bytes
 *
 * @param disk_info
 * @param block
 * @param buffer
 * @param bytes
 * @param offset
 */
void writeBlockBytes(DiskInfo* disk_info, int64_t block, int8_t* buffer, int64_t bytes,
                     int64_t offset) {
  printf("[W block block=%li bytes=%li offset=%li] ", block, bytes, offset);
  writeBytes(disk_info, block * disk_info->block_size + offset, buffer, bytes);
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
  printf("{R INode INode=%i} ", number);
  readBlockBytes(disk_info, group_desc.bg_inode_table, (int8_t*)i_node, sizeof(INode),
                 table_index * sizeof(INode));
}

/**
 * @brief Writes an INode to the disk
 *
 * @param disk_info
 * @param number
 * @param i_node
 */
void writeINode(DiskInfo* disk_info, int32_t number, INode* i_node) {
  GroupDesc group_desc;
  int32_t   block       = (number - 1) / disk_info->inodes_per_group;
  int32_t   table_index = (number - 1) % disk_info->inodes_per_group;

  readGroupDesc(disk_info, block, &group_desc);
  printf("{W INode INode=%i} ", number);
  writeBlockBytes(disk_info, group_desc.bg_inode_table, (int8_t*)i_node, sizeof(INode),
                  table_index * sizeof(INode));
}

/**
 * @brief Just read all of the data from an INode
 *
 * @param disk_info
 * @param inode
 */
void readINodeData(DiskInfo* disk_info, INode* inode, int8_t* buffer, int32_t bytes) {
  int32_t buffer_offset = 0;

  // TODO: Support links
  for (int pos = 0; pos < 15; pos++) {
    if (inode->i_block[pos] == 0) {
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
 * @brief Writes a dir to disk
 *
 * @param disk_info
 * @param inode
 * @param directory
 * @param offset
 * @return int
 */
int writeDirectory(DiskInfo* disk_info, INode* inode, Directory* directory, int32_t offset) {
  int32_t block_index     = offset / disk_info->block_size;
  int32_t directory_index = offset % disk_info->block_size;
  int32_t padding         = 0;

  printf("{W Directory offset=%i (block_index=%i directory_index=%i)} ", offset, block_index,
         directory_index);

  // Write the first bit of the struct to the disk
  writeBlockBytes(disk_info, inode->i_block[block_index], (int8_t*)directory, 8, directory_index);

  // Write the name into the directory
  writeBlockBytes(disk_info, inode->i_block[block_index], (int8_t*)directory->name,
                  directory->name_len, directory_index + 8);

  // Add padding if required
  if (directory_index % 4 != 0) {
    int8_t padding_data[] = { 0, 0, 0, 0 };
    padding               = 4 - (directory_index % 4);
    writeBlockBytes(disk_info, inode->i_block[block_index], (int8_t*)&padding_data, padding,
                    directory_index + 8 + directory->name_len);
  }

  return 8 + directory->name_len + padding;
}

/**
 * @brief Searches the disk for a path
 *
 * @param state
 * @param parameter
 * @return Path*
 */
Path* readPath(State* state, char* parameter) {
  
}