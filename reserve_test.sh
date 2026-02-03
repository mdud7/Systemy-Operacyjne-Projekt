#!/bin/bash
rm -f raport_bar.txt

(sleep 2; echo "2"; sleep 1; echo "5"; sleep 2; echo "0") | ./main > /dev/null & PID=$!
wait $PID

if grep -q "Zarezerwowano" raport_bar.txt; then
    echo "test zaliczony: Rezerwacja wykryta w logach."
else
    echo "test failed: brak wpisu o rezerwacji w logach."
fi