/**
 * @file hello.c
 * @brief Simple Hello World program.
 *
 * This program prints "Hello World!" along with the board name
 * defined by CONFIG_BOARD.
 *
 * @author Dharm Kapatel
 * @date 2025
 */

#include <stdio.h>
#include "header.h"

/**
 * @brief Print Hello World with board name.
 *
 * This function demonstrates a basic printf usage
 * in C. It appends the configured board name to
 * the "Hello World!" string.
 */
void hello_print(void)
{
    printf("Hello World! %s\n", CONFIG_BOARD);
}
