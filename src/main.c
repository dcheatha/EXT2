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

  Path* root_path         = (Path*)calloc(1, sizeof(Path));
  root_path->inode_number = EXT2_ROOT_INO;
  strcpy(root_path->name, "/");

  state->path_root = root_path;
  state->path_cwd  = root_path;
}

/**
 * @brief Grabs a command from STDIN
 *
 * @param parameter
 * @return int32_t
 */
uint32_t grabCommand(char parameter[EXT2_NAME_LEN]) {
  char command[EXT2_NAME_LEN] = { 0 };
  char buffer[EXT2_NAME_LEN]  = { 0 };

  scanf("%[^\n]%*c", buffer);

  // Split the string, dump part after command into parameter
  for (int32_t pos = 0; pos < EXT2_NAME_LEN; pos++) {
    if (buffer[pos] == '\0') {
      bzero(parameter, EXT2_NAME_LEN);
      strcpy(command, buffer);
      break;
    }

    if (buffer[pos] == ' ') {
      buffer[pos] = '\0';
      strcpy(command, buffer);
      strcpy(parameter, buffer + pos + 1);
      break;
    }
  }

  // Get the command id
  for (int32_t pos = 0; pos < sizeof(kPrintCommands) / sizeof(char*); pos++) {
    if (strcasecmp(command, kPrintCommands[pos]) == 0) {
      return pos;
    }
  }

  return -1;
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

  printf("Mounting disk=%s\n", *(argv + 1));

  disk_info.file_desc = open(*(argv + 1), O_RDWR);

  if (disk_info.file_desc == 0) {
    printf("Unable to open file=%s\n", *(argv + 1));
    return EXIT_FAILURE;
  }

  State state = { &ext_info, &disk_info };
  initalizeState(&state);

  uint32_t command_id;
  char     parameter[EXT2_NAME_LEN];

  while (1) {
    bzero(parameter, sizeof(parameter));

    // printMenu();

    printf("gid=%d uid=%d> ", state.user.group_id, state.user.user_id);
    command_id = grabCommand(parameter);

    if (command_id > kCommandCount) {
      printf("shell: command not found\n");
      continue;
    }

    // Run the command
    runCommand(&state, (Command)command_id, parameter);
  }

  return EXIT_SUCCESS;
}