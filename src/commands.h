#ifndef COMMANDS_H
#define COMMANDS_H

#include "types.h"
#include "utility.h"

/**
 * @brief Runs a command on the filesystem
 *
 * @param state
 * @param command
 * @param parameter
 */
void runCommand(State* state, Command command, char* parameter);

#endif