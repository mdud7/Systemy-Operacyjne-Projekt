#!/bin/bash
rm -f raport_bar.txt

(sleep 15; echo "0") | ./main > /dev/null & PID=$!
wait $PID

IN=$(grep -a "Wchodze do baru jako proces" raport_bar.txt | wc -l)
OUT_RESIGN=$(grep -a "Rezygnacja na wejsciu" raport_bar.txt | wc -l)
OUT_SUCCESS=$(grep -a "skonczyla jesc, wychodzimy" raport_bar.txt | wc -l)

TOTAL_OUT=$((OUT_RESIGN + OUT_SUCCESS))

echo "Weszlo: $IN"
echo "Wyszlo: $TOTAL_OUT (Rezygnacja: $OUT_RESIGN, Sukces: $OUT_SUCCESS)"

if [ "$IN" -eq "$TOTAL_OUT" ] && [ "$IN" -gt 0 ]; then
    echo "test zaliczony: bilans sie zgadza."
else
    echo "test niezaliczony: Bilans nierowny."
fi