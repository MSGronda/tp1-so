#!/bin/bash

gcc -Wall -std=c99 -g md5.c -o md5 -lm -lrt -pthread
