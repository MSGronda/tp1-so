#ifndef _LIB_H_
#define _LIB_H_


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


/*
 * << is_regular_file >>
 * ----------------------------------------------------------------------
 * Description: Checks is a provided path is a regular file
 * ----------------------------------------------------------------------
 * Receives: 
 *      [path] = path to a given file
 * Returns:
 *      1 in case it is a regular file
 *      0 if not
 */
int is_regular_file(const char *path);

#endif