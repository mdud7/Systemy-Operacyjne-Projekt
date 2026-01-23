#!/bin/bash

FILE="raport_bar.txt"

if [ ! -f "$FILE" ]; then
    echo "Blad: Plik $FILE nie istnieje."
    exit 1
fi

echo "INFO"
echo "Klienci (stolik): $(grep -c "zajmujemy miejsce" $FILE)"
echo "Zaplacono:        $(grep -c "zaplacono" $FILE)"
echo "Kolejka:          $(grep -c "Brak wolnych" $FILE)"
echo "Rezygnacje:       $(grep -c "rezygnujemy" $FILE)"
echo "Wyszlo:           $(grep -c "wychodzimy" $FILE)"
