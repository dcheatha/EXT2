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
  ioINode(state->disk_info, &first_item_inode, folder_to_remove.inode, IOMODE_READ);
  ioDirectoryEntry(state->disk_info, &first_item, &first_item_inode, (8 + 1) + (8 + 2),
                   IOMODE_READ);

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
  /*
  Directory parent_folder;
  Directory source_file;
  Directory dest_file;

  char source[EXT2_NAME_LEN];
  char dest[EXT2_NAME_LEN];

  char* token = strtok(parameter, " ");
  strcpy(source, token);
  token = strtok(parameter, " ");
  strcpy(dest, token);

  if (readPath(state, source, &source_file) == EXIT_FAILURE) {
    printf("cp: %s: No such file or directory\n", source);
    return;
  }

  if (source_file.file_type != EXT2_FT_REG_FILE) {
    printf("cp: %s: Is not a regular file\n", source);
    return;
  }

  strcpy(dest_file.name, dest);
  dest_file.name_len  = strlen(dest_file.name);
  dest_file.file_type = EXT2_FT_REG_FILE;
  dest_file.rec_len   = 8 + strlen(dest_file.name);
  dest_file.inode     = allocateINode(state);

  allocateDirectoryEntry(state->disk_info, parent_folder.inode, &dest_file);
  */
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
void runLINK(State* state, char* parameter) {}

/**
 * @brief Unlinks a link
 *
 * @param state
 * @param parameter
 */
void runUNLINK(State* state, char* parameter) {}

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
void runCD(State* state, char* parameter) { printMenu(); }

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
 * @brief Runs a command on the filesystem
 *
 * @param command
 * @param parameter
 */
void runCommand(State* state, Command command, char* parameter) {
  void (*commands[])(State * state,
                     char* parameter) = { runLS,     runMKDIR,    runRMDIR,    runCREATE, runLINK,
                                          runUNLINK, runMKFS,     runCAT,      runCP,     runMENU,
                                          runCD,     runDISKINFO, runINODEINFO };
  (*commands[command])(state, parameter);
}