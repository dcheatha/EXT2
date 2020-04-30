#ifndef FIND_H
#define FIND_H

#include "io.h"

/**
 * @brief Searches the filesystem for a path
 *
 * @param state
 * @param found_file
 * @param parameter
 * @return int32_t
 */
int32_t findPath(State* state, Directory* found_file, char* parameter);

/**
 * @brief Find Path Parent
 *
 * @param state
 * @param found_file
 * @param parameter
 * @return int32_t
 */
int32_t findPathParent(State* state, Directory* found_file, char* parameter);

/**
 * @brief Path Exists
 *
 * @param state
 * @param parameter
 * @return int8_t
 */
int8_t pathExists(State* state, char* parameter);

#endif