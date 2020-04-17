#include "commands.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Initalize application state
 *
 * @param state
 */
void initalizeState(State* state) {
  initializeFilesystem(state->disk_info, state->ext_info);

  state->user.user_id  = getuid();
  state->user.group_id = getgid();

  Path* root_path  = (Path*)calloc(1, sizeof(Path));
  root_path->inode_number = EXT2_ROOT_INO;
  strcpy(root_path->name, "/");

  state->path_root = root_path;
}

/**
 * @brief Simulates the EXT2 Filesystem
 *
 * @param argc
 * @param argv
 */
int32_t main(int32_t argc, char** argv) {
  ExtInfo  ext_info;
  DiskInfo disk_info;

  if (argc < 1) {
    printf("Please provide a disk image\n");
    return EXIT_FAILURE;
  }

  disk_info.file_desc = open(*(argv + 1), O_RDWR);

  if (disk_info.file_desc == 0) {
    printf("Unable to open file=%s\n", *(argv + 1));
    return EXIT_FAILURE;
  }

  State state{ &ext_info, &disk_info };
  initalizeState(&state);

  uint32_t command_id;
  char     parameter[255];

  while (1) {
    bzero(parameter, sizeof(parameter));

    printMenu();
    printf("Please choose a command: ");
    scanf("%u", &command_id);
    command_id += 1;

    if (command_id > kCommandCount) {
      printf("Command %u does not exist.\n", command_id - 1);
      continue;
    }

    printf("Please enter a parameter: ");
    scanf("%s", &parameter);

    // Run the command
    runCommand(&state, (Command)command_id, parameter);
  }

  return EXIT_SUCCESS;
}