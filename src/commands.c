#include "commands.h"

/**
 * @brief Runs LS
 *
 * @param parameter
 */
void runLS(State* state, char* parameter) {
  Directory found_file;

  if (readPath(state, parameter, &found_file) == EXIT_FAILURE) {
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

  if (readPathExists(state, parameter) == EXIT_SUCCESS) {
    printf("mkdir: cannot create directory: '%s': File exists\n", parameter);
    return;
  }

  if (readPathParent(state, parameter, &parent_folder) == EXIT_FAILURE) {
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
  Directory folder_to_remove;

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
  if (readPathExists(state, parameter) == EXIT_FAILURE) {
    printf("rmdir: %s: No such file or directory\n", parameter);
    return;
  }

  // Load it if the path exists
  readPath(state, parameter, &folder_to_remove);

  // Make sure we're removing a dir
  if (folder_to_remove.file_type != EXT2_FT_DIR) {
    printf("rmdir: %s: Not a directory\n", parameter);
    return;
  }

  // Make sure it's an empty dir
  INode     first_item_inode;
  Directory first_item;
  readINode(state->disk_info, folder_to_remove.inode, &first_item_inode);
  readDirectory(state->disk_info, &first_item_inode, &first_item, (8 + 1) + (8 + 2));

  if (!isEndDirectory(&first_item)) {
    printf("rmdir: %s: Directory not empty\n", parameter);
    return;
  }

  // Now we can dealloc the dir
}

/**
 * @brief Cats a file
 *
 * @param state
 * @param parameter
 */
void runCAT(State* state, char* parameter) {
  Directory found_file;

  if (readPath(state, parameter, &found_file) == EXIT_FAILURE) {
    printf("cat: %s: No such file or directory\n", parameter);
    return;
  }

  if (found_file.file_type != EXT2_FT_REG_FILE) {
    printf("cat: %s: Is not a regular file\n", parameter);
    return;
  }

  INode inode;
  readINode(state->disk_info, found_file.inode, &inode);

  printFile(state->disk_info, &inode);
}

void runCREATE(State* state, char* parameter) {}
void runLINK(State* state, char* parameter) {}
void runUNLINK(State* state, char* parameter) {}
void runMKFS(State* state, char* parameter) {}
void runCP(State* state, char* parameter) {}

/**
 * @brief Prints the menu
 *
 * @param state
 * @param parameter
 */
void runMENU(State* state, char* parameter) { printMenu(); }

/**
 * @brief Runs a command on the filesystem
 *
 * @param command
 * @param parameter
 */
void runCommand(State* state, Command command, char* parameter) {
  void (*commands[])(State * state, char* parameter) = { runLS,   runMKDIR,  runRMDIR, runCREATE,
                                                         runLINK, runUNLINK, runMKFS,  runCAT,
                                                         runCP,   runMENU };
  (*commands[command])(state, parameter);
}