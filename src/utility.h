#ifndef UTILITY_H
#define UTILITY_H

#include "alloc.h"
#include "io.h"
#include "print.h"
#include "types.h"

#include <stdlib.h>

/**
 * @brief Reads and prepares data structures for Filesystem
 *
 * @param disk_info
 * @param ext_info
 */
void initializeFilesystem(DiskInfo* disk_info, ExtInfo* ext_info);

/**
 * @brief Checks if a bit is true
 *
 * @param byte
 * @param bit
 * @return int
 */
int testBit(int8_t byte, int8_t bit);

/**
 * @brief Finds a free bit in a bitmap
 *
 * @param data
 * @param bit to start at
 * @return int
 */
int findFreeBit(int8_t data, int8_t start);

/**
 * @brief Like strtok, but not useless
 *
 * @param destination of size EXT2_NAME_LEN
 * @param input
 * @param offset
 * @param is_dir 1 if dir 0 otherwise
 */
void parsePath(char* destination, char* input, int32_t* offset, int8_t* is_dir);

/**
 * @brief Clears the path
 *
 * @param state
 * @param path
 */
void clearPath(State* state, Path* path);

#endif