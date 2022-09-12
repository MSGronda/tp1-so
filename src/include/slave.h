#ifndef _SLAVE_H_
#define _SLAVE_H_

#include "defs.h"
#include "resource_manager.h"

#include <signal.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <semaphore.h>

/*
 * << slave >>
 * ----------------------------------------------------------------------
 * Description: Slave is connected via two pipes to a process. 
 *              It calculates the md5 of a recieved file (app_to_slave)
 *              Returns the result through the other pipe (slave_to_app)
 * ----------------------------------------------------------------------
 * Recieves: 
 *      [app_to_slave] = Array containg the fd of the (App -> Slave) pipe
 *      [slave_to_a] = Array containg the fd of the (App -> Slave) pipe
 * Returns:
 *      0 if no errors occur
 */
int slave(int * app_to_slave, int * slave_to_app);

#endif