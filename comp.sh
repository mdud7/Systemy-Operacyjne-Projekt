#!/bin/bash

gcc main.c -o main -pthread
gcc client.c -o client -pthread
gcc cashier.c -o cashier -pthread
gcc staff.c -o staff -pthread
gcc menager.c -o menager -pthread
gcc generator.c -o generator -pthread