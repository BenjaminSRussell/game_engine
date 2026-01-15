#!/bin/bash

# Configuration
NETWORKING_DIR="Source/Runtime/Core/Networking"
SOURCE_DIR="$NETWORKING_DIR/Private"
PUBLIC_DIR="$NETWORKING_DIR/Public"
CORE_DIR="Source/Runtime/Core"
MEMORY_DIR="$CORE_DIR/Memory"
LOGGER_DIR="$CORE_DIR/Logging"

INCLUDE_FLAGS="-I./Source/Runtime/Core/Include -I./$PUBLIC_DIR -I./$CORE_DIR/Memory/Public -I./$CORE_DIR/Logging/Public -I./src/engine/include"

OUTPUT_BIN="verify_networking_bin"

echo "Verifying Networking integration..."

# Compile
clang -std=c11 -Wall -Wextra -Werror \
    $INCLUDE_FLAGS \
    -DLOG_USE_COLOR \
    "$SOURCE_DIR/networking_socket.c" \
    "$SOURCE_DIR/networking_core.c" \
    "$SOURCE_DIR/networking_connection.c" \
    "$SOURCE_DIR/networking_manager.c" \
    "$SOURCE_DIR/networking_replication.c" \
    "$SOURCE_DIR/networking_update.c" \
    "$SOURCE_DIR/networking_test_main.c" \
    "$MEMORY_DIR/unified_memory.c" \
    "$LOGGER_DIR/unified_logger.c" \
    -pthread \
    -o $OUTPUT_BIN

# Check result
if [ $? -eq 0 ]; then
    echo "Networking System compiled successfully."
    echo "Running Networking Verification..."
    ./$OUTPUT_BIN
    RET_CODE=$?
    rm $OUTPUT_BIN
    exit $RET_CODE
else
    echo "Networking System compilation FAILED."
    exit 1
fi
