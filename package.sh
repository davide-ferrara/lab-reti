#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

for dir in "$SCRIPT_DIR"/ho*/; do
    name="$(basename "$dir")"
    zipname="${name}_518629.zip"

    files=()

    # sorgenti
    while IFS= read -r -d '' f; do
        files+=("$f")
    done < <(find "$dir" -maxdepth 1 \( -name "*.c" -o -name "*.h" -o -name "Makefile" \) -print0)

    # report pdf
    pdf="$(find "$dir/report" -maxdepth 1 -name "*.pdf" 2>/dev/null | head -1)"
    if [[ -n "$pdf" ]]; then
        files+=("$pdf")
    fi

    if [[ ${#files[@]} -eq 0 ]]; then
        echo "[$name] nessun file trovato, skip"
        continue
    fi

    zip -j "$SCRIPT_DIR/$zipname" "${files[@]}"
    echo "[$name] -> $zipname (${#files[@]} file)"
done
