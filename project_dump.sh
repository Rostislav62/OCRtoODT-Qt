#!/bin/bash

# ========== CONFIG ==========
OUTPUT_FILE="project_dump.txt"
ROOT_DIR="."   # можно заменить на абсолютный путь к проекту
# ============================

echo "Generating project dump into $OUTPUT_FILE ..."
echo "" > "$OUTPUT_FILE"

# ---- 1. STRUCTURE (tree) ----
{
    echo "====================== PROJECT DIRECTORY TREE ======================"
    echo ""
    tree -al "$ROOT_DIR"
    echo ""
    echo "********************************************************************"
    echo ""
} >> "$OUTPUT_FILE"

# ---- 2. FILE LIST ----
{
    echo "====================== FULL FILE LIST =============================="
    echo ""
    find "$ROOT_DIR" -type f \
        ! -path "*/build/*" \
        ! -path "*/.git/*" \
        ! -path "*/.idea/*" \
        ! -path "*/cmake-build-*/*"
    echo ""
    echo "********************************************************************"
    echo ""
} >> "$OUTPUT_FILE"

# ---- FUNCTION: append file contents ----
dump_file() {
    local file="$1"

    # Полный путь
    local fullpath
    fullpath="$(realpath "$file")"

    {
        echo "====================== FILE: $fullpath ============================="
        echo ""
        cat "$file"
        echo ""
        echo "********************************************************************"
        echo ""
    } >> "$OUTPUT_FILE"
}

export -f dump_file

# ---- 3. DUMP FILE CONTENTS ----
find "$ROOT_DIR" -type f \
    ! -name "$OUTPUT_FILE" \
    ! -path "*/build/*" \
    ! -path "*/.git/*" \
    ! -path "*/.idea/*" \
    ! -path "*/cmake-build-*/*" \
    -print0 | while IFS= read -r -d '' file; do
        dump_file "$file"
    done

echo "Done."

