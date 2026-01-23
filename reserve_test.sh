#!/bin/bash

killall -9 main staff menager cashier generator client 2>/dev/null
rm -f raport_bar.txt

echo "TEST SYGNALU 2 (REZERWACJA)"

(sleep 1; echo "2"; sleep 1; echo "4"; sleep 2; echo "0") | ./main > /dev/null 2>&1 &

sleep 6
killall -9 main staff menager cashier generator client 2>/dev/null

if grep -q "Zarezerwowano" raport_bar.txt; then
    echo "SYGNAL 2 (REZERWACJA): OK"
    grep "Zarezerwowano" raport_bar.txt
else
    echo "SYGNAL 2: FAIL"
fi
