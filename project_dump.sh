#!/bin/bash

# ================= CONFIG =================
OUTPUT_FILE="project_dump.txt"
ROOT_DIR="."
# ==========================================

echo "Generating clean project dump into $OUTPUT_FILE ..."
echo "" > "$OUTPUT_FILE"

# --- Настройки включения ---
INCLUDE_DIRS=("ui" "src" "settings" "dialogs")
ROOT_FILES=("CMakeLists.txt" "config.yaml")

# ------------------------------------------
# 1. FILE LIST
# ------------------------------------------
{
    echo "====================== FILE LIST ======================"
    echo ""

    # Корневые файлы
    for f in "${ROOT_FILES[@]}"; do
        if [ -f "$ROOT_DIR/$f" ]; then
            echo "$f"
        fi
    done

    # Полные папки
    for dir in "${INCLUDE_DIRS[@]}"; do
        if [ -d "$ROOT_DIR/$dir" ]; then
            find "$ROOT_DIR/$dir" -type f
        fi
    done

    # docs → только *.md
    if [ -d "$ROOT_DIR/docs" ]; then
        find "$ROOT_DIR/docs" -type f -name "*.md"
    fi

    # resources → только *.qrc
    if [ -d "$ROOT_DIR/resources" ]; then
        find "$ROOT_DIR/resources" -type f -name "*.qrc"
    fi

    echo ""
    echo "======================================================="
    echo ""
} >> "$OUTPUT_FILE"


# ------------------------------------------
# FUNCTION: dump file content
# ------------------------------------------
dump_file() {
    local file="$1"

    {
        echo "====================== FILE: $file ======================"
        echo ""
        cat "$file"
        echo ""
        echo "=========================================================="
        echo ""
    } >> "$OUTPUT_FILE"
}

export -f dump_file


# ------------------------------------------
# 2. DUMP FILE CONTENTS
# ------------------------------------------

# Корневые файлы
for f in "${ROOT_FILES[@]}"; do
    if [ -f "$ROOT_DIR/$f" ]; then
        dump_file "$ROOT_DIR/$f"
    fi
done

# Полные папки
for dir in "${INCLUDE_DIRS[@]}"; do
    if [ -d "$ROOT_DIR/$dir" ]; then
        find "$ROOT_DIR/$dir" -type f -print0 | \
        while IFS= read -r -d '' file; do
            dump_file "$file"
        done
    fi
done

# docs → только md
if [ -d "$ROOT_DIR/docs" ]; then
    find "$ROOT_DIR/docs" -type f -name "*.md" -print0 | \
    while IFS= read -r -d '' file; do
        dump_file "$file"
    done
fi

# resources → только qrc
if [ -d "$ROOT_DIR/resources" ]; then
    find "$ROOT_DIR/resources" -type f -name "*.qrc" -print0 | \
    while IFS= read -r -d '' file; do
        dump_file "$file"
    done
fi

echo "Done."
