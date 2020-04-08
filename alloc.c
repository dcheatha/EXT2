#include "alloc.h"

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
    if (read_index % 4 != 0) {
      read_index += 4 - (read_index % 4);
    }

    read_index += readDirectory(disk_info, &root_inode, &root_dir, read_index);

    if (isEndDirectory(&root_dir)) {
      // We've found the end dir! Now we overrwrite it with the new dir:
      write_index = read_index - 8 - root_dir.name_len;
      write_index = writeDirectory(disk_info, &root_inode, directory, write_index);
      // Now write the end dir back:
      write_index = writeDirectory(disk_info, &root_inode, &root_dir, write_index);
      // Now increase iNode link count
      root_inode.i_links_count++;
      root_inode.i_atime = now;
      // And write the INode back to disk:
      writeINode(disk_info, inode_start, &root_inode);
    }
  }
}

/**
 * @brief Returns the INode no of a newly allocated INode
 *
 * @param disk_info
 * @param ext_info
 * @return int32_t
 */
int32_t allocateINode(DiskInfo* disk_info, ExtInfo* ext_info) {
  GroupDesc group_desc;
  int32_t   group_pos = 0;
  int8_t    buffer[disk_info->block_size];

  for (int32_t group = 0; group < disk_info->group_count; group++) {
    readGroupDesc(disk_info, group, &group_desc);
    readBlock(disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer);

    for (int32_t pos = 0; pos < disk_info->inodes_per_group / 8; pos++) {
      int32_t bit = findFreeBit(buffer[pos], 0);

      if (bit != -1) {
        int32_t free_inode_pos = bit + (pos * 8) + (group * disk_info->inodes_per_group);
        printf("found free inode=%i\n", free_inode_pos);
        return;
      }
    }
  }
}