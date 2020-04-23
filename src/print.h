#ifndef PRINT_H
#define PRINT_H

#include "alloc.h"
#include "io.h"
#include "types.h"

#include <stdio.h>

/**
 * @brief List of commands as strings
 */
static const char* kPrintCommands[] = { "LS",     "MKDIR", "RMDIR", "CREATE", "LINK",
                                        "UNLINK", "MKFS",  "CAT",   "CP",     "MENU" };

/**
 * @brief Count of commands
 */
static const int32_t kCommandCount = sizeof(kPrintCommands) / sizeof(int64_t);

/**
 * @brief Prints the menu
 */
void printMenu();

/**
 * @brief Prints basic ext2 info
 *
 * @param ext_info
 * @param disk_info
 */
void printDiskInfomation(ExtInfo* ext_info, DiskInfo* disk_info);

/**
 * @brief Prints an INode
 *
 * @param inode
 */
void printINode(INode* inode);

/**
 * @brief Prints info for a single dir entry
 *
 * @param directory
 */
void printDirectory(DiskInfo* disk_info, Directory* directory);

/**
 * @brief Prints an entire dir table
 *
 * @param disk_info
 * @param inode_start
 */
void printDirectoryTable(DiskInfo* disk_info, int32_t inode_start);

/**
 * @brief Prints a bitmap
 *
 * @param bitmap
 */
void printBitmap(int8_t bitmap);

/**
 * @brief Prints a group desc
 *
 * @param group_desc
 */
void printGroupDesc(GroupDesc* group_desc);

/**
 * @brief Prints a file
 *
 * @param disk_info
 * @param file
 */
void printFile(DiskInfo* disk_info, INode* file);

#endif