#include "utility.h"

/**
 * @brief Loads common filesystem infomation and prepares data structures
 *
 * Layout of each block group:
 * | Super Block | Group Desc | Block Bitmap | INode Bitmap | INode Table | Data
 * Blocks
 *
 * @param disk_info
 * @param ext_info
 */
void initializeFilesystem(DiskInfo* disk_info, ExtInfo* ext_info) {
  // The superblock is located at an offset of 1024 bytes, ie, the first block.
  ioBytes(disk_info, (int8_t*)&ext_info->super_block, sizeof(struct ext2_super_block),
          SUPERBLOCK_OFFSET, IOMODE_READ);

  if (ext_info->super_block.s_magic != EXT2_SUPER_MAGIC) {
    printf("Magical error with s_magic=%x (is this an EXT2 filesystem?)\n",
           ext_info->super_block.s_magic);
    exit(EXIT_FAILURE);
  }

  // Store block size (needs to be 32bits to the left...)
  disk_info->block_size       = 1024 << ext_info->super_block.s_log_block_size;
  disk_info->s_log_block_size = ext_info->super_block.s_log_block_size;

  // Need to know how many blocks there are in a group
  disk_info->blocks_per_group = ext_info->super_block.s_blocks_per_group;

  // Need to know how many inodes there are in a group
  disk_info->inodes_per_group = ext_info->super_block.s_inodes_per_group;

  // Can't figure out how to get this number. Blocks Count returns the size of
  // the filesystem in megabytes which makes no sense whatsoever.
  disk_info->group_count = 2;

  // The group descriptor (for the first group) is the block right after the
  // superblock, so read that in. Depending on the block size, it could be in
  // the 2nd or 3rd block.
  if (disk_info->block_size <= SUPERBLOCK_OFFSET) {
    // Read the 3rd block
    // readBytes(disk_info, disk_info->block_size * 2,
    //          (int8_t *)&ext_info->super_block, sizeof(struct
    //          ext2_group_desc));
  } else {
    // Read the 2nd block
    // readBytes(disk_info, disk_info->block_size,
    //          (int8_t *)&ext_info->super_block, sizeof(struct
    //          ext2_group_desc));
  }

  // printDiskInfomation(ext_info, disk_info);
  // printGroupDesc(ext_info->)
  // printDirectoryTable(disk_info, EXT2_ROOT_INO);
}

/**
 * @brief Check if a bit is true
 *
 * @param byte
 * @param bit
 * @return int
 */
int32_t testBit(int8_t data, int8_t bit) {
  bit = 1 << bit;
  return (bit & data) != 0;
}

/**
 * @brief Finds a free bit in a bitmap
 *
 * @param data
 * @return int
 */
int32_t findFreeBit(int8_t data, int8_t start) {
  for (int8_t bit_pos = start; bit_pos < 8 * sizeof(int8_t); bit_pos++) {
    if (!testBit(data, bit_pos)) {
      return bit_pos;
    }
  }

  return -1;
}

/**
 * @brief Parses a path
 * Like strtok, but for paths and tells you when the path is about to be over.
 *
 * @param destination of size EXT2_NAME_LEN
 * @param input
 * @param offset
 * @param is_dir
 */
void parsePath(char* destination, char* input, int32_t* offset, int8_t* is_more) {
  bzero(destination, EXT2_NAME_LEN);

  *is_more = 0;

  for (int32_t pos = *offset; pos < strlen(input); pos++) {
    if (input[pos] == '/') {
      *offset = pos + 1;

      if (pos + 1 < strlen(input)) {
        *is_more = 1;
      }

      return;
    }

    if (pos == *offset) {
      bzero(destination, EXT2_NAME_LEN);
    }

    destination[pos - *offset] = input[pos];
  }
}

/**
 * @brief Clears the path of the state
 *
 * @param state
 * @param path
 */
void clearPath(State* state, Path* path) {
  Path* current_pos = path;

  while (current_pos != state->path_root) {
    current_pos = current_pos->parent;
    free(current_pos->child);
  }

  state->path_cwd = state->path_root;
}

/**
 * @brief Get the Parameter Stub
 *
 * @param parameter
 * @param stub
 */
void getParameterStub(char* parameter, char* stub) {
  int32_t parameter_len = strlen(parameter);

  for (int pos = parameter_len; pos > 0; pos--) {
    if (parameter[pos] == '/') {
      strncpy(stub, parameter + pos + 1, parameter_len - pos - 1);
      return;
    }
  }
  strcpy(stub, parameter);
}

/**
 * @brief Get the Default Mode of an INode
 *
 * @param file_type
 * @return int16_t
 */
int16_t getDefaultMode(int16_t file_type) {
  switch (file_type) {
    case EXT2_FT_DIR: return EXT2_S_IFDIR | EXT2_S_IRWXU | EXT2_S_IRWXG;
    default: return EXT2_S_IRWXU | EXT2_S_IRWXG;
  }
}

/**
 * @brief Calculates the indirect ranges
 *
 * @param disk_info
 * @return IndirectRange
 */
IndirectRange calculateIndirectRange(DiskInfo* disk_info) {
  int32_t       indirects_per_block = disk_info->block_size / sizeof(int32_t);
  IndirectRange range               = { 0, EXT2_INDIRECT_SINGLE };

  range.double_start = range.single_start + indirects_per_block;
  range.triple_start = range.double_start + (indirects_per_block * indirects_per_block);
  range.triple_end =
    range.triple_start + (indirects_per_block * indirects_per_block * indirects_per_block);

  range.indirects_per_block = indirects_per_block;

  return range;
}