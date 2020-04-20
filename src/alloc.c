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
 * @brief Appends a dir table with a new entry
 *
 * @param disk_info
 * @param inode_start
 * @param directory
 */
void allocateDirectoryEntry(DiskInfo* disk_info, int32_t inode_start, Directory* directory) {
  INode     root_inode;
  Directory root_dir;
  int32_t   read_index  = 0;
  int32_t   write_index = 0;
  time_t    now         = time(NULL);

  readINode(disk_info, inode_start, &root_inode);

  while (1) {
    // Read index must be a multiple of 4 to start a directory entry
    if (read_index % 4 != 0) {
      read_index += 4 - (read_index % 4);
    }

    read_index += readDirectory(disk_info, &root_inode, &root_dir, read_index);

    if (isEndDirectory(&root_dir)) {
      // We've found the end dir! Now we overrwrite it with the new dir:

      // Jump to the start of the end dir:
      write_index = read_index - 8 - root_dir.name_len;

      // Write the new directory entry to the disk:
      write_index = writeDirectory(disk_info, &root_inode, directory, write_index);

      // Now write the end dir back:
      write_index = writeDirectory(disk_info, &root_inode, &root_dir, write_index);

      // Now increase INode link count (The root dir now links to it)
      root_inode.i_links_count++;
      root_inode.i_atime = now;
      // And write the INode back to disk:
      writeINode(disk_info, inode_start, &root_inode);
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
int32_t allocateINode(DiskInfo* disk_info, ExtInfo* ext_info) {
  GroupDesc group_desc;
  int8_t    buffer[disk_info->block_size];

  for (int32_t group = 0; group < disk_info->group_count; group++) {
    readGroupDesc(disk_info, group, &group_desc);
    readBlock(disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer);

    for (int32_t pos = 0; pos < disk_info->inodes_per_group / 8; pos++) {
      int32_t bit = findFreeBit(buffer[pos], 0);

      if (bit != -1) {
        int32_t free_inode_pos = 1 + bit + (pos * 8) + (group * disk_info->inodes_per_group);

        // Mark bitmap as used, thus alloc'ing it
        buffer[pos] ^= (1 << pos);

        // Just write the entire block back to the disk
        writeBlock(disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer);

        // Now we know which INode we have
        return free_inode_pos;
      }
    }
  }

  return -1;
}