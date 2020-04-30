#ifndef UTILITY_H
#define UTILITY_H

#include "alloc.h"
#include "io.h"
#include "print.h"
#include "types.h"

#include <stdlib.h>
#include <strings.h>

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
int32_t testBit(int8_t byte, int8_t bit);

/**
 * @brief Finds a free bit in a bitmap
 *
 * @param data
 * @param bit to start at
 * @return int
 */
int32_t findFreeBit(int8_t data, int8_t start);

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

/**
 * @brief Get the Parameter Stub object
 *
 * @param parameter
 * @param stub
 */
void getParameterStub(char* parameter, char* stub);

/**
 * @brief Get the Default Mode of an INode
 *
 * @param file_type
 * @return int16_t
 */
int16_t getDefaultMode(int16_t file_type);

/**
 * @brief Calculates the INode indirection ranges
 *
 * @param disk_info
 * @return IndirectRange
 */
IndirectRange calculateIndirectRange(DiskInfo* disk_info);

#endif