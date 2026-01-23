#!/bin/bash

rm -f raport_bar.txt

{ echo "1"; sleep 1; } | ./main > /dev/null 2>&1 &
sleep 2
killall -9 main staff menager cashier generator client 2>/dev/null

grep -q "Podwojono" raport_bar.txt && echo " MANAGER: OK" || echo " MANAGER: FAIL"