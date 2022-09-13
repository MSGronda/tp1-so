#ifndef _APP_H_
#define _APP_H_


#define _POSIX_SOURCE
#define _BSD_SOURCE 
#define _XOPEN_SOURCE 501

#include "defs.h"
#include "errors.h"
#include "resource_manager.h"
#include "slave.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>


/*
 * << exit_handler >>
 * ----------------------------------------------------------------------
 * Description: Frees memory before exitting, this function is then passed
 * to on_exit(), that´s why it has [code] and [arr].
 * ----------------------------------------------------------------------
 * Receives: --
 * Returns:--
 */
void exit_handler(int code, void * val);


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