#!/bin/bash
rm -f raport_bar.txt

(sleep 2; echo "1"; sleep 2; echo "0") | ./main > /dev/null & PID=$!
wait $PID

if grep -q "Podwojono stoliki" raport_bar.txt; then
    echo "test zaliczony: Stoliki podwojone."
else
    echo "test failed: Brak loga o podwojeniu."
fi