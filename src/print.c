#include "print.h"

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

void printDirectoryTable(DiskInfo* disk_info, int32_t inode_start) {
  INode     root_inode;
  Directory root_dir;
  int32_t   dir_index = 0;

  readINode(disk_info, inode_start, &root_inode);
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
      // printINode(&data_inode);

      char buffer[1024] = { 0 };
      readINodeData(disk_info, &data_inode, (int8_t*)&buffer, 1024);
      printf("%20s: %s\n", "Data", buffer);
    }
  }
}
