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
  // printf("(R bytes bytes=%li offset=%i)\n", bytes, offset);
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
  // printf("(W bytes bytes=%li offset=%i)\n", bytes, offset);
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
  // printf("[R block block=%li] ", block);
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
  // printf("[W block block=%li] ", block);
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
  // printf("[R block block=%li bytes=%li offset=%li] ", block, bytes, offset);

  if (bytes + offset > disk_info->block_size) {
    printf(
      "io: readBlockBytes(): warn: Read past block boundaries on block %ld from bytes %ld to %ld "
      "\n",
      block, offset, offset + bytes);
  }

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
  // printf("[W block block=%li bytes=%li offset=%li] ", block, bytes, offset);

  if (bytes + offset > disk_info->block_size) {
    printf(
      "io: writeBlockBytes(): warn: Wrote past block boundaries on block %ld from bytes %ld to %ld "
      "\n",
      block, offset, offset + bytes);
  }

  writeBytes(disk_info, block * disk_info->block_size + offset, buffer, bytes);
}

/**
 * @brief Determines if a group has a copy of the superblock
 *
 * @param group
 * @return int
 */
int32_t groupHasSuperBlock(const int32_t group) {
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
  // printf("{R INode INode=%i} ", number);
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
  // printf("{W INode INode=%i} ", number);
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
  for (int32_t pos = 0; pos < 15; pos++) {
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
int32_t readDirectory(DiskInfo* disk_info, INode* inode, Directory* directory, int32_t offset) {
  int32_t block_index     = offset / disk_info->block_size;
  int32_t directory_index = offset % disk_info->block_size;

  // printf("{R Directory offset=%i (block=%i directory_index=%i)}\n", offset,
  //       inode->i_block[block_index], directory_index);

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
int32_t writeDirectory(DiskInfo* disk_info, INode* inode, Directory* directory, int32_t offset) {
  int32_t block_index     = offset / disk_info->block_size;
  int32_t directory_index = offset % disk_info->block_size;
  int32_t padding         = 0;

  // printf("{W Directory offset=%i (block=%i directory_index=%i)}\n", offset,
  //       inode->i_block[block_index], directory_index);

  // Calculate the rec len
  directory->rec_len = 8 + directory->name_len;

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
 * @brief Searches the disk for a path from the current path
 * Writes the Directory of the item on EXIT_SUCCESS
 *
 * @param state
 * @param parameter
 * @param found_file File that was found
 * @return int32_t
 */
int32_t readPath(State* state, char* parameter, Directory* found_file) {
  // We want the current dir:
  if (strlen(parameter) <= 0) {
    INode inode;
    readINode(state->disk_info, state->path_cwd->inode_number, &inode);
    readDirectory(state->disk_info, &inode, found_file, 0);
    return EXIT_SUCCESS;
  }

  // We want root so grab the dir of root:
  if (parameter[0] == '/' && strlen(parameter) == 1) {
    clearPath(state, state->path_cwd);
    INode inode;
    readINode(state->disk_info, EXT2_ROOT_INO, &inode);
    readDirectory(state->disk_info, &inode, found_file, 0);
    return EXIT_SUCCESS;
  }

  // We're searching from root, so clear our CWD:
  if (parameter[0] == '/') {
    clearPath(state, state->path_cwd);
  }

  // Prep our path object
  Path current_path;
  memcpy(&current_path, state->path_cwd, sizeof(Path));
  current_path.parent = NULL;
  current_path.child  = NULL;

  // Get ready to search the dir tables
  char    item_name[EXT2_NAME_LEN];
  int8_t  is_more          = 1;
  int32_t parameter_offset = 0;

  // Prepare to read the disk
  INode     current_inode;
  Directory current_directory;
  int32_t   directory_offset = 0;

  // Get the first bit of the path

  do {
    parsePath(item_name, parameter, &parameter_offset, &is_more);
    readINode(state->disk_info, current_path.inode_number, &current_inode);
    directory_offset +=
      readDirectory(state->disk_info, &current_inode, &current_directory, directory_offset);

    // Search until the end for a matching name
    while (strcmp(current_directory.name, item_name) != 0 && !isEndDirectory(&current_directory)) {
      if (directory_offset % 4 != 0) {
        directory_offset += 4 - (directory_offset % 4);
      }

      directory_offset +=
        readDirectory(state->disk_info, &current_inode, &current_directory, directory_offset);
    }

    // If we couldn't find a match and we searched until the end
    if (isEndDirectory(&current_directory)) {
      return EXIT_FAILURE;
    }

    // If we somehow got an item we wanna dig into, switch to its dir table and reset our
    // dir_offset:
    bzero(&current_path, sizeof(Path));
    current_path.inode_number = current_directory.inode;
    strncpy(current_path.name, current_directory.name, current_directory.name_len);
    directory_offset = 0;
  } while (is_more);

  memcpy(found_file, &current_directory, sizeof(Directory));

  return EXIT_SUCCESS;
}

/**
 * @brief Like readPath, but returns the parent of the path instead
 *
 * @param state
 * @param parameter
 * @param found_file
 * @return int32_t
 */
int32_t readPathParent(State* state, char* parameter, Directory* found_file) {
  int32_t parameter_length                = strlen(parameter);
  char    parameter_parent[EXT2_NAME_LEN] = { 0 };

  for (int32_t pos = parameter_length; pos > 0; pos--) {
    if (parameter[pos] == '/') {
      strncpy(parameter_parent, parameter, pos);
      break;
    }
  }

  return readPath(state, parameter_parent, found_file);
}

/**
 * @brief Checks if a path exists
 *
 * @param state
 * @param parameter
 * @return int8_t
 */
int8_t readPathExists(State* state, char* parameter) {
  Directory file;
  return readPath(state, parameter, &file);
}

/**
 * @brief Reads a file from the disk into buffer for n bytes
 *
 * @param disk_info
 * @param inode
 * @param buffer
 * @param bytes
 * @param offset (in bytes)
 */
void readFile(DiskInfo* disk_info, INode* inode, int8_t* buffer, int32_t bytes,
              int32_t offset_bytes) {
  int32_t blocks_to_read =
    bytes / disk_info->block_size + (bytes % disk_info->block_size != 0);  // Rounds value up
  int32_t offset_blocks = offset_bytes / disk_info->block_size;

  // Set upperbound to amount of blocks the INode has
  if (blocks_to_read > inode->i_blocks) {
    blocks_to_read = inode->i_blocks;
  }

  if (blocks_to_read + offset_blocks > inode->i_blocks) {
    printf(
      "io: readFile(): warn: Requested to read blocks %d to %d when there are only %d blocks\n",
      offset_blocks, blocks_to_read + offset_blocks, inode->i_blocks);
  }

  IndirectRange range      = calculateIndirectRange(disk_info);
  int32_t       buffer_pos = 0;

  for (int32_t block_pos = offset_blocks;
       block_pos < blocks_to_read + offset_blocks && block_pos < inode->i_blocks; block_pos++) {
    int32_t read_bytes  = disk_info->block_size;  // Bytes to read from this block
    int32_t read_offset = 0;                      // Offset in bytes to read from this block
    int32_t block_no    = 0;                      // Block to read

    // If we are the first read, adjust for our offset:
    if (block_pos == offset_blocks) {
      read_bytes -= offset_bytes;
      read_offset += offset_bytes;
    }

    // If we are the last read, adjust for our offset:
    if (block_pos == blocks_to_read + offset_blocks) {
      read_bytes -= offset_bytes;
    }

    // Triple Indirect range:
    if (block_pos >= range.triple_start) {
      int32_t block_offset =
        sizeof(int32_t) * ((block_pos - range.triple_start) /
                           (range.indirects_per_block * range.indirects_per_block));

      readBlockBytes(disk_info, inode->i_block[EXT2_INDIRECT_TRIPLE], (int8_t*)&block_no,
                     sizeof(int32_t), block_offset);
    }

    // Double Indirect range:
    if (block_pos >= range.double_start) {
      int32_t block_offset =
        sizeof(int32_t) * ((block_pos - range.double_start) / (range.indirects_per_block));

      // If triple indirect was NOT called, then work off of double indirect table
      if (!(block_pos >= range.triple_start)) {
        block_no = inode->i_block[EXT2_INDIRECT_DOUBLE];
      }

      readBlockBytes(disk_info, block_no, (int8_t*)&block_no, sizeof(int32_t), block_offset);
    }

    // Single Indirect range:
    if (block_pos >= range.single_start) {
      int32_t block_offset = sizeof(int32_t) * (block_pos - range.single_start);

      // If double indirect was NOT called, then work off of single indirect table
      if (!(block_pos >= range.double_start)) {
        block_no = inode->i_block[EXT2_INDIRECT_SINGLE];
      }

      // Read which block we want from the single indirect table:
      readBlockBytes(disk_info, block_no, (int8_t*)&block_no, sizeof(block_no), block_offset);
    }

    // Direct block range:
    if (block_pos < range.single_start) {
      block_no = inode->i_block[block_pos];
    }

    readBlockBytes(disk_info, block_no, buffer + buffer_pos, read_bytes, read_offset);
    buffer_pos += read_bytes;

    if (block_pos > range.triple_end) {
      printf("io: readFile(): warn: Requested block beyond max supported range\n");
    }
  }
}
