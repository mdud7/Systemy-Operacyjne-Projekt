#!/bin/bash

killall -9 main staff menager cashier generator client 2>/dev/null
rm -f raport_bar.txt

echo "TEST SYGNALU 3 (POZAR)"

(sleep 3; echo "3"; sleep 2; echo "0") | ./main > /dev/null 2>&1 &

sleep 6
killall -9 main staff menager cashier generator client 2>/dev/null

if grep -q -E "alarm|uciekamy|pozar" raport_bar.txt; then
    echo "SYGNAL 3 (POZAR): OK - Klienci uciekli."
    echo "log:"
    grep -E "alarm|uciekamy|pozar" raport_bar.txt | head -n 10
else
    echo "SYGNAL 3: FAIL"
fi