#!/bin/bash

golden_file="./llfi/baseline/golden_std_output"
directory="./llfi/std_output"

if [[ ! -f "$golden_file" ]]; then
    echo "Error: Golden file '$golden_file' not found."
    exit 1
fi

golden_last=$(tail -n 1 "$golden_file")

checked=0
different=0

for file in "$directory"/*; do
    [[ -f "$file" ]] || continue
    [[ "$file" -ef "$golden_file" ]] && continue

    ((checked++))

    last_line=$(tail -n 1 "$file")

    if [[ "$last_line" != "$golden_last" ]]; then
        echo "Different: $file"
        ((different++))
    fi
done

echo
echo "Total Number of Runs: $checked"
echo "Number of SDCs: $different"

