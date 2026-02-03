#!/bin/bash

rm -f raport_bar.txt

(sleep 2; echo "3"; sleep 1) | ./main > /dev/null & PID=$!
wait $PID

if grep -q "OGLOSZONO POZAR" raport_bar.txt; then
    echo "test zaliczony: Wykryto pozar."
else
    echo "test failed: brak reakcji na pozar w logach."
fi