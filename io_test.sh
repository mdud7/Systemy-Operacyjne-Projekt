#!/bin/bash

killall -9 main staff menager cashier generator client 2>/dev/null
rm -f raport_bar.txt

echo "Symulacja baru (15 sekund)..."

(sleep 15; echo "0") | ./main > /dev/null 2>&1 &

sleep 15

pkill -u $(whoami) -f generator
killall -u $(whoami) -9 main staff menager cashier client 2>/dev/null

IN=$(grep -c "zajmujemy" raport_bar.txt)
OUT=$(grep -c -E "wychodzimy|wyszlo" raport_bar.txt)

echo "Statystyki: Weszlo: $IN | Wyszlo: $OUT"

if [ "$IN" -eq "$OUT" ] && [ "$IN" -gt 0 ]; then
    echo "OK"
else
    echo "ZLE"
fi