
/**
 * @brief Searches the disk for a path from the current path
 * Writes the Directory of the item on EXIT_SUCCESS
 *
 * @param state
 * @param parameter
 * @param found_file File that was found
 * @return int32_t
 */
int32_t findPath(State* state, char* parameter, Directory* found_file) {
  // We want the current dir:
  if (strlen(parameter) <= 0) {
    INode inode;
    readINode(state->disk_info, state->path_cwd->inode_number, &inode);
    readDirectory(state->disk_info, &inode, found_file, 0);
    return EXIT_SUCCESS;
  }

  // We want root so grab the dir of root:
  if (parameter[0] == '/' && strlen(parameter) == 1) {
    clearPath(state, state->path_cwd);
    INode inode;
    readINode(state->disk_info, EXT2_ROOT_INO, &inode);
    readDirectory(state->disk_info, &inode, found_file, 0);
    return EXIT_SUCCESS;
  }

  // We're searching from root, so clear our CWD:
  if (parameter[0] == '/') {
    clearPath(state, state->path_cwd);
  }

  // Prep our path object
  Path current_path;
  memcpy(&current_path, state->path_cwd, sizeof(Path));
  current_path.parent = NULL;
  current_path.child  = NULL;

  // Get ready to search the dir tables
  char    item_name[EXT2_NAME_LEN];
  int8_t  is_more          = 1;
  int32_t parameter_offset = 0;

  // Prepare to read the disk
  INode     current_inode;
  Directory current_directory;
  int32_t   directory_offset = 0;

  // Get the first bit of the path

  do {
    parsePath(item_name, parameter, &parameter_offset, &is_more);
    readINode(state->disk_info, current_path.inode_number, &current_inode);
    directory_offset +=
      readDirectory(state->disk_info, &current_inode, &current_directory, directory_offset);

    // Search until the end for a matching name
    while (strcmp(current_directory.name, item_name) != 0 && !isEndDirectory(&current_directory)) {
      if (directory_offset % 4 != 0) {
        directory_offset += 4 - (directory_offset % 4);
      }

      directory_offset +=
        readDirectory(state->disk_info, &current_inode, &current_directory, directory_offset);
    }

    // If we couldn't find a match and we searched until the end
    if (isEndDirectory(&current_directory)) {
      return EXIT_FAILURE;
    }

    // If we somehow got an item we wanna dig into, switch to its dir table and reset our
    // dir_offset:
    bzero(&current_path, sizeof(Path));
    current_path.inode_number = current_directory.inode;
    strncpy(current_path.name, current_directory.name, current_directory.name_len);
    directory_offset = 0;
  } while (is_more);

  memcpy(found_file, &current_directory, sizeof(Directory));

  return EXIT_SUCCESS;
}