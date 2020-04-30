#include "print.h"

/**
 * @brief Prints the menu
 */
void printMenu() {
  printf("shell: ");
  for (int32_t pos = 0; pos < kCommandCount; pos++) {
    printf("%s ", kPrintCommands[pos]);
  }
  printf("\n");
}

/**
 * @brief Dumps infomation about the disk for debugging
 *
 * @param ext_info
 */
void printDiskInfomation(ExtInfo* ext_info, DiskInfo* disk_info) {
  printf("%17s: %10s\n", "Volume Name", ext_info->super_block.s_volume_name);
  printf("%17s: %10s\n", "Last Mount Path", ext_info->super_block.s_last_mounted);

  printf("%17s: %10u\n", "Blocks per Group", ext_info->super_block.s_blocks_per_group);
  printf("%17s: %10li\n", "Blocks Count",
         (int64_t)ext_info->super_block.s_blocks_count_hi << 32 |
           ext_info->super_block.s_blocks_count);
  printf("%17s: %10li\n", "Free Blocks Count",
         (int64_t)ext_info->super_block.s_free_blocks_hi << 32 |
           ext_info->super_block.s_free_blocks_count);
  printf("%17s: %10u\n", "Block Size", 1024 << ext_info->super_block.s_log_block_size);
  printf("%17s: %10u\n", "First Block", ext_info->super_block.s_first_data_block);
  printf("%17s: %10u\n", "INode Count", ext_info->super_block.s_inodes_count);
  printf("%17s: %10u\n", "First INode", ext_info->super_block.s_first_ino);
  printf("%17s: %10u\n", "Free INodes", ext_info->super_block.s_free_inodes_count);
  printf("%17s: %10u\n", "INodes per Group", ext_info->super_block.s_inodes_per_group);
}

/**
 * @brief Prints a group description
 *
 * @param group_desc
 */
void printGroupDesc(GroupDesc* group_desc) {
  printf("Group Desc: {\n");
  printf("%17s: %10i\n", "Free Blocks", group_desc->bg_free_blocks_count);
  printf("%17s: %10i\n", "Free INodes", group_desc->bg_free_inodes_count);
  printf("%17s: %10i\n", "Used Dir Count", group_desc->bg_used_dirs_count);
  printf("%17s: %10i\n", "Block Bitmap", group_desc->bg_block_bitmap);
  printf("%17s: %10i\n", "INode Bitmap", group_desc->bg_inode_bitmap);
  printf("}\n");
}

/**
 * @brief Prints the mode of a file
 *
 * @param mode
 */
void printMode(uint16_t mode) {
  static const char perms[] = { 'r', 'w', 'x' };

  if (mode & 0x4000) {
    printf("d");
  } else {
    printf("-");
  }

  // There are 9 total modes
  for (int32_t pos = 0; pos < 9; pos++) {
    uint16_t current_value = 0x100 >> (pos - 0);
    if (mode & current_value) {
      printf("%c", perms[pos % 3]);
    } else {
      printf("-");
    }
  }
}

/**
 * @brief Prints a dir
 *
 * @param directory
 */
void printDirectory(DiskInfo* disk_info, Directory* directory) {
  INode inode;

  // Pull the INode up from the disk
  ioINode(disk_info, &inode, directory->inode, IOMODE_READ);

  struct tm* time_struct;
  char       formatted_time[20] = { 0 };

  time_struct = localtime((time_t*)&inode.i_mtime);
  strftime(formatted_time, 20, "%d %b %H:%M", time_struct);

  printf("%3d ", inode.i_blocks);
  printMode(inode.i_mode);
  printf("%3d ", inode.i_links_count);
  printf("%5d ", inode.i_uid);
  printf("%5d ", inode.i_gid);
  printf("%10li ", (int64_t)inode.i_size_high << 32 | inode.i_size);
  printf("%s ", formatted_time);
  printf("%s\n", directory->name);
}

/**
 * @brief Prints an INode
 *
 * @param inode
 */
void printINode(INode* inode) {
  printf("%10s: %10i\n", "Size", inode->i_size);
  printf("%10s: %10i\n", "Blocks", inode->i_blocks);

  for (int32_t pos = 0; pos < 15; pos++) {
    printf("%5s[%3i]: %10i\n", "Block", pos, inode->i_block[pos]);
  }

  printf("%10s: ", "Mode");
  printMode(inode->i_mode);
  printf("\n");
}

/**
 * @brief Prints a directory table
 *
 * @param disk_info
 * @param inode_start
 */
void printDirectoryTable(DiskInfo* disk_info, int32_t inode_start) {
  INode     root_inode;
  Directory root_dir;
  int64_t   directory_offset = 0;

  ioINode(disk_info, &root_inode, inode_start, IOMODE_READ);

  while (1) {
    if (directory_offset % 4 != 0) {
      directory_offset += 4 - (directory_offset % 4);
    }

    directory_offset +=
      ioDirectoryEntry(disk_info, &root_dir, &root_inode, directory_offset, IOMODE_READ);

    if (isEndDirectory(&root_dir)) {
      break;
    }

    printDirectory(disk_info, &root_dir);
  }
}

/**
 * @brief Prints a bitmap
 *
 * @param bitmap
 */
void printBitmap(int8_t bitmap) {
  printf("0b");

  for (int8_t bit_pos = 0; bit_pos < 8 * sizeof(int8_t); bit_pos++) {
    printf("%i", testBit(bitmap, bit_pos));
  }
}

/**
 * @brief Prints a file
 *
 * @param disk_info
 * @param file
 */
void printFile(DiskInfo* disk_info, INode* file) {
  int8_t* seriously_the_entire_file = (int8_t*)calloc(sizeof(int8_t), file->i_size);

  ioFile(disk_info, seriously_the_entire_file, file, file->i_size, 0, IOMODE_READ);

  printf("%s", seriously_the_entire_file);

  free(seriously_the_entire_file);
}