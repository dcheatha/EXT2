#include "commands.h"

/**
 * @brief Runs LS
 *
 * @param parameter
 */
void runLS(State* state, char* parameter) {
  Directory found_file;

  if (findPath(state, &found_file, parameter) == EXIT_FAILURE) {
    printf("ls: %s: No such file or directory\n", parameter);
    return;
  }

  if (found_file.file_type != EXT2_FT_DIR) {
    printDirectory(state->disk_info, &found_file);
    return;
  }

  printDirectoryTable(state->disk_info, found_file.inode);
}

/**
 * @brief Runs MKDIR
 *
 * @param state
 * @param parameter
 */
void runMKDIR(State* state, char* parameter) {
  Directory parent_folder;

  if (pathExists(state, parameter) == EXIT_SUCCESS) {
    printf("mkdir: cannot create directory: '%s': File exists\n", parameter);
    return;
  }

  if (findPathParent(state, &parent_folder, parameter) == EXIT_FAILURE) {
    printf("mkdir: cannot create directory: '%s': No such file or directory\n", parameter);
    return;
  }

  Directory new_dir;
  getParameterStub(parameter, new_dir.name);

  new_dir.name_len  = strlen(new_dir.name);
  new_dir.file_type = EXT2_FT_DIR;
  new_dir.rec_len   = 8 + strlen(new_dir.name);

  allocateDirectoryTable(state, &parent_folder, &new_dir);
  allocateDirectoryEntry(state->disk_info, parent_folder.inode, &new_dir);
}

/**
 * @brief Runs MKDIR
 *
 * @param state
 * @param parameter
 */
void runRMDIR(State* state, char* parameter) {
  Directory parent_folder;
  Directory folder_to_remove;
  INode     to_remove_inode;

  // Make sure it's not something that would be bad to remove
  char stub[EXT2_NAME_LEN];
  getParameterStub(parameter, stub);

  char* bad_matches[] = { ".", "..", "/" };

  for (int32_t pos = 0; pos < sizeof(bad_matches) / sizeof(char*); pos++) {
    if (strcmp(stub, bad_matches[pos]) == 0) {
      printf("rmdir: \".\", \"..\", and \"/\" may not be removed");
      return;
    }
  }

  // Check and see if the path exists
  if (pathExists(state, parameter) == EXIT_FAILURE) {
    printf("rmdir: %s: No such file or directory\n", parameter);
    return;
  }

  // Load it if the path exists
  findPath(state, &folder_to_remove, parameter);
  findPathParent(state, &parent_folder, parameter);

  // Make sure we're removing a dir
  if (folder_to_remove.file_type != EXT2_FT_DIR) {
    printf("rmdir: %s: Not a directory\n", parameter);
    return;
  }

  // Make sure it's an empty dir
  INode     first_item_inode;
  Directory first_item;
  int64_t   read_index = 0;
  ioINode(state->disk_info, &first_item_inode, folder_to_remove.inode, IOMODE_READ);

  for (int pos = 0; pos < 2; pos++) {
    read_index +=
      ioDirectoryEntry(state->disk_info, &first_item, &first_item_inode, read_index, IOMODE_READ);

    if (read_index % 4 != 0) {
      read_index += 4 - (read_index % 4);
    }
  }

  read_index +=
    ioDirectoryEntry(state->disk_info, &first_item, &first_item_inode, read_index, IOMODE_READ);

  if (!isEndDirectory(&first_item)) {
    printf("rmdir: %s: Directory not empty\n", parameter);
    return;
  }

  // Store the INode in memory before we start killing things:
  ioINode(state->disk_info, &to_remove_inode, folder_to_remove.inode, IOMODE_READ);

  // Now we can dealloc the dir
  deallocateDirectoryEntry(state->disk_info, parent_folder.inode, stub);

  // And get rid of the INode!
  deallocateINode(state->disk_info, folder_to_remove.inode);
}

/**
 * @brief Cats a file
 *
 * @param state
 * @param parameter
 */
void runCAT(State* state, char* parameter) {
  Directory found_file;

  if (findPath(state, &found_file, parameter) == EXIT_FAILURE) {
    printf("cat: %s: No such file or directory\n", parameter);
    return;
  }

  if (found_file.file_type != EXT2_FT_REG_FILE) {
    printf("cat: %s: Is not a regular file\n", parameter);
    return;
  }

  INode inode;
  ioINode(state->disk_info, &inode, found_file.inode, IOMODE_READ);

  printFile(state->disk_info, &inode);
}

/**
 * @brief Copys a file
 *
 * @param state
 * @param parameter
 */
void runCP(State* state, char* parameter) {
  Directory parent_folder;
  Directory source_file;
  Directory dest_file;

  char source[EXT2_NAME_LEN];
  char dest[EXT2_NAME_LEN];

  char* token = strtok(parameter, " ");
  strcpy(dest, token);
  token = strtok(NULL, " ");

  if (token == NULL) {
    printf("cp: Must specify two paths\n");
  }

  strcpy(source, token);

  if (findPath(state, &source_file, source) == EXIT_FAILURE) {
    printf("cp: %s: No such file or directory\n", source);
    return;
  }

  findPathParent(state, &parent_folder, parameter);

  if (source_file.file_type != EXT2_FT_REG_FILE) {
    printf("cp: %s: Is not a regular file\n", source);
    return;
  }

  // Dump the dir name to the disk
  strcpy(dest_file.name, dest);
  dest_file.name_len  = strlen(dest_file.name);
  dest_file.file_type = EXT2_FT_REG_FILE;
  dest_file.rec_len   = 8 + strlen(dest_file.name);
  dest_file.inode     = allocateINode(state);

  // Load the INodes we're about to copy
  INode dest_inode;
  INode source_inode;
  ioINode(state->disk_info, &dest_inode, dest_file.inode, IOMODE_READ);
  ioINode(state->disk_info, &source_inode, source_file.inode, IOMODE_READ);

  if (state->disk_info->free_blocks < source_inode.i_blocks) {
    printf("cp: Not enough free space (need %ld more blocks)\n",
           source_inode.i_blocks - state->disk_info->free_blocks);
    return;
  }

  allocateDirectoryEntry(state->disk_info, parent_folder.inode, &dest_file);
  // Allocate all of the blocks our dest INode will require:
  allocateINodeBlocks(state->disk_info, &dest_inode, source_inode.i_blocks);

  // Lazily just load the entire file into ram:
  int8_t* file_contents = (int8_t*)calloc(sizeof(int8_t), source_inode.i_size);

  ioFile(state->disk_info, file_contents, &source_inode, source_inode.i_size, 0, IOMODE_READ);
  ioFile(state->disk_info, file_contents, &dest_inode, source_inode.i_size, 0, IOMODE_WRITE);

  dest_inode.i_size = source_inode.i_size;

  ioINode(state->disk_info, &dest_inode, dest_file.inode, IOMODE_WRITE);
}

/**
 * @brief Creates a file?
 *
 * @param state
 * @param parameter
 */
void runCREATE(State* state, char* parameter) {
  Directory parent_folder;

  if (pathExists(state, parameter) == EXIT_SUCCESS) {
    printf("create: cannot create file: '%s': File exists\n", parameter);
    return;
  }

  if (findPathParent(state, &parent_folder, parameter) == EXIT_FAILURE) {
    printf("create: cannot create file: '%s': No such file or directory\n", parameter);
    return;
  }

  Directory new_file;
  getParameterStub(parameter, new_file.name);

  new_file.name_len  = strlen(new_file.name);
  new_file.file_type = EXT2_FT_REG_FILE;
  new_file.rec_len   = 8 + strlen(new_file.name);
  new_file.inode     = allocateINode(state);

  allocateDirectoryEntry(state->disk_info, parent_folder.inode, &new_file);
}

/**
 * @brief Links a link
 *
 * @param state
 * @param parameter
 */
void runLINK(State* state, char* parameter) {
  Directory parent_folder;
  Directory source_file;
  Directory dest_file;

  char source[EXT2_NAME_LEN];
  char dest[EXT2_NAME_LEN];

  char* token = strtok(parameter, " ");
  strcpy(dest, token);
  token = strtok(NULL, " ");

  if (token == NULL) {
    printf("link: Must specify two paths\n");
  }

  strcpy(source, token);

  if (findPath(state, &source_file, source) == EXIT_FAILURE) {
    printf("link: %s: No such file or directory\n", source);
    return;
  }

  findPathParent(state, &parent_folder, parameter);

  INode source_inode;
  ioINode(state->disk_info, &source_inode, source_file.inode, IOMODE_READ);
  source_inode.i_links_count++;
  ioINode(state->disk_info, &source_inode, source_file.inode, IOMODE_WRITE);

  strcpy(dest_file.name, dest);
  dest_file.name_len  = strlen(dest_file.name);
  dest_file.file_type = source_file.file_type;
  dest_file.rec_len   = 8 + strlen(dest_file.name);
  dest_file.inode     = source_file.inode;

  allocateDirectoryEntry(state->disk_info, parent_folder.inode, &dest_file);
}

/**
 * @brief Unlinks a link
 *
 * @param state
 * @param parameter
 */
void runUNLINK(State* state, char* parameter) {
  Directory parent_folder;
  Directory to_unlink;

  if (findPath(state, &to_unlink, parameter) == EXIT_FAILURE) {
    printf("unlink: %s: No such file or directory\n", parameter);
    return;
  }

  findPathParent(state, &parent_folder, parameter);

  INode source_inode;
  ioINode(state->disk_info, &source_inode, to_unlink.inode, IOMODE_READ);
  source_inode.i_links_count--;
  ioINode(state->disk_info, &source_inode, to_unlink.inode, IOMODE_WRITE);

  if (source_inode.i_links_count <= 0) {
    deallocateINode(state->disk_info, to_unlink.inode);
  }

  deallocateDirectoryEntry(state->disk_info, parent_folder.inode, to_unlink.name);
}

/**
 * @brief Inits the filesystem
 *
 * @param state
 * @param parameter
 */
void runMKFS(State* state, char* parameter) {}

/**
 * @brief Prints the menu
 *
 * @param state
 * @param parameter
 */
void runMENU(State* state, char* parameter) { printMenu(); }

/**
 * @brief Changes dir
 *
 * @param state
 * @param parameter
 */
void runCD(State* state, char* parameter) {
  Directory found_file;

  if (findPath(state, &found_file, parameter) == EXIT_FAILURE) {
    printf("cd: %s: No such file or directory\n", parameter);
    return;
  }

  if (found_file.file_type != EXT2_FT_DIR) {
    printf("cd: %s: Not a directory\n", parameter);
    return;
  }

  char    current_folder[EXT2_NAME_LEN] = { '\0' };
  int32_t offset                        = 0;
  int8_t  is_more                       = 1;
  Path*   current_path                  = state->path_cwd;

  while (is_more) {
    parsePath(current_folder, parameter, &offset, &is_more);
    Path* new_path   = (Path*)calloc(1, sizeof(Path));
    new_path->parent = current_path;
    strcpy(new_path->name, current_folder);

    current_path->child = new_path;

    current_path = new_path;
  }

  current_path->inode_number = found_file.inode;

  state->path_cwd = current_path;
}

/**
 * @brief Prints Block Bitmap
 *
 * @param state
 * @param parameter
 */
void runBLOCKBITMAP(State* state, char* parameter) {
  GroupDesc group_desc;
  int8_t    buffer[state->disk_info->block_size];

  int64_t block_count = (int64_t)state->ext_info->super_block.s_blocks_count_hi << 32 |
                        state->ext_info->super_block.s_blocks_count;

  printf("      ");

  for (int32_t pos = 0; pos < 8; pos++) {
    printf("%-9d", pos * 8);
  }

  for (int32_t group = 0; group < state->disk_info->group_count; group++) {
    ioGroupDescriptor(state->disk_info, &group_desc, group, IOMODE_READ);
    ioBlock(state->disk_info, group_desc.bg_block_bitmap, (int8_t*)&buffer, IOMODE_READ);

    for (int32_t pos = 0; pos < state->disk_info->blocks_per_group / 8; pos++) {
      int64_t real_block_pos = 8 * (pos + group * (state->disk_info->blocks_per_group / 8));

      if (block_count < real_block_pos) {
        break;
      }

      if (pos % 8 == 0) {
        printf("\n%5ld ", real_block_pos);
      }

      printBitmap(buffer[pos]);
      printf(" ");
    }
  }

  printf("\n");
}

/**
 * @brief Prints INode bitmap
 *
 * @param state
 * @param parameter
 */
void runINODEBITMAP(State* state, char* parameter) {
  GroupDesc group_desc;
  int8_t    buffer[state->disk_info->block_size];

  int64_t block_count = state->ext_info->super_block.s_inodes_count;

  printf("      ");

  for (int32_t pos = 0; pos < 8; pos++) {
    printf("%-9d", pos * 8);
  }

  for (int32_t group = 0; group < state->disk_info->group_count; group++) {
    ioGroupDescriptor(state->disk_info, &group_desc, group, IOMODE_READ);
    ioBlock(state->disk_info, group_desc.bg_inode_bitmap, (int8_t*)&buffer, IOMODE_READ);

    for (int32_t pos = 0; pos < state->disk_info->inodes_per_group / 8; pos++) {
      int64_t real_block_pos = 8 * (pos + group * (state->disk_info->inodes_per_group / 8));

      if (block_count < real_block_pos) {
        break;
      }

      if (pos % 8 == 0) {
        printf("\n%5ld ", real_block_pos);
      }

      printBitmap(buffer[pos]);
      printf(" ");
    }
  }

  printf("\n");
}

/**
 * @brief Views an indirect block
 *
 * @param state
 * @param parameter
 */
void runRAWBLOCK(State* state, char* parameter) {
  int64_t block_no = atoi(parameter);

  int32_t buffer[state->disk_info->block_size / sizeof(int32_t)];

  if (block_no == 0) {
    return;
  }

  ioBlock(state->disk_info, block_no, (int8_t*)&buffer, IOMODE_READ);

  for (int32_t pos = 0; pos < state->disk_info->block_size / sizeof(int32_t); pos++) {
    if (pos % 8 == 0) {
      printf("\n%4d:", pos);
    }

    printf("%11d ", buffer[pos]);
  }

  printf("\n");
}

/**
 * @brief Prints the current disk info
 *
 * @param state
 * @param parameter
 */
void runDISKINFO(State* state, char* parameter) {
  printDiskInfomation(state->ext_info, state->disk_info);
}

void runINODEINFO(State* state, char* parameter) {
  Directory found_file;
  INode     inode;

  if (findPath(state, &found_file, parameter) == EXIT_FAILURE) {
    printf("inode: %s: No such file or directory\n", parameter);
    return;
  }

  ioINode(state->disk_info, &inode, found_file.inode, IOMODE_READ);

  printINode(&inode);
}

/**
 * @brief Prints WD
 *
 * @param state
 * @param parameter
 */
void runPWD(State* state, char* parameter) {
  Path* current_pos = state->path_root->child;

  printf("/");

  while (current_pos != state->path_cwd) {
    printf("%s/", current_pos->name);
    current_pos = current_pos->child;
  }
  printf("%s", current_pos->name);

  printf("\n");
}

/**
 * @brief Runs a command on the filesystem
 *
 * @param command
 * @param parameter
 */
void runCommand(State* state, Command command, char* parameter) {
  void (*commands[])(State * state, char* parameter) = {
    runLS,        runMKDIR,       runRMDIR,       runCREATE,   runLINK, runUNLINK,
    runMKFS,      runCAT,         runCP,          runMENU,     runCD,   runDISKINFO,
    runINODEINFO, runBLOCKBITMAP, runINODEBITMAP, runRAWBLOCK, runPWD
  };
  (*commands[command])(state, parameter);
}