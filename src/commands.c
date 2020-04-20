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
 * @brief Runs a command on the filesystem
 *
 * @param command
 * @param parameter
 */
void runCommand(State* state, Command command, char* parameter) {
  void (*commands[])(State * state, char* parameter) = { runLS };
  printf("command=%d parameter=%s\n", command, parameter);
  (*commands[command])(state, parameter);
}