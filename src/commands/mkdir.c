#include "commands.h"

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