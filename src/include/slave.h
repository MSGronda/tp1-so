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

int slave(int * app_to_slave, int * slave_to_app);

#endif