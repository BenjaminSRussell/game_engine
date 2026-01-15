#!/bin/bash

# Configuration
SCRIPTING_DIR="Source/Runtime/Core/Scripting"
SOURCE_DIR="$SCRIPTING_DIR/Private"
PUBLIC_DIR="$SCRIPTING_DIR/Public"
CORE_DIR="Source/Runtime/Core"
MEMORY_DIR="$CORE_DIR/Memory"
LOGGER_DIR="$CORE_DIR/Logging"

INCLUDE_FLAGS="-I./Source/Runtime/Core/Include -I./$PUBLIC_DIR -I./$CORE_DIR/Memory/Public -I./$CORE_DIR/Logging/Public -I./src/engine/include"

OUTPUT_BIN="verify_scripting_bin"

echo "Verifying Scripting integration..."

# Compile
clang -std=c11 -Wall -Wextra -Werror \
    $INCLUDE_FLAGS \
    -DLOG_USE_COLOR \
    "$SOURCE_DIR/scripting_core.c" \
    "$SOURCE_DIR/scripting_commands.c" \
    "$SOURCE_DIR/scripting_variables.c" \
    "$SOURCE_DIR/scripting_console.c" \
    "$SOURCE_DIR/scripting_visual.c" \
    "$SOURCE_DIR/scripting_test_main.c" \
    "$MEMORY_DIR/unified_memory.c" \
    "$LOGGER_DIR/unified_logger.c" \
    -o $OUTPUT_BIN

# Check result
if [ $? -eq 0 ]; then
    echo "Scripting System compiled successfully."
    echo "Running Scripting Verification..."
    ./$OUTPUT_BIN
    RET_CODE=$?
    rm $OUTPUT_BIN
    exit $RET_CODE
else
    echo "Scripting System compilation FAILED."
    exit 1
fi
