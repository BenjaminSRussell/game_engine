#!/bin/bash

# Configuration
SOURCE_DIR="Source/Runtime/Core/ECS/Private"
PUBLIC_DIR="Source/Runtime/Core/ECS/Public"
CORE_DIR="Source/Runtime/Core"
MEMORY_DIR="Source/Runtime/Core/Memory"
LOGGER_DIR="Source/Runtime/Core/Logging"

INCLUDE_FLAGS="-I./Source/Runtime/Core/Include -I./$PUBLIC_DIR -I./$CORE_DIR/Memory/Public -I./$CORE_DIR/Logging/Public -I./src/engine/include"

OUTPUT_BIN="verify_ecs_bin"

echo "Verifying ECS integration..."

# Compile
clang -std=c11 -Wall -Wextra -Werror \
    $INCLUDE_FLAGS \
    -DLOG_USE_COLOR \
    "$SOURCE_DIR/ecs_core.c" \
    "$SOURCE_DIR/ecs_entity.c" \
    "$SOURCE_DIR/ecs_component.c" \
    "$SOURCE_DIR/ecs_archetype.c" \
    "$SOURCE_DIR/ecs_query.c" \
    "$SOURCE_DIR/ecs_system.c" \
    "$MEMORY_DIR/unified_memory.c" \
    "$LOGGER_DIR/unified_logger.c" \
    "$SOURCE_DIR/ecs_test_main.c" \
    -o $OUTPUT_BIN

# Check result
if [ $? -eq 0 ]; then
    echo "ECS System compiled successfully."
    echo "Running ECS Verification..."
    ./$OUTPUT_BIN
    RET_CODE=$?
    rm $OUTPUT_BIN
    exit $RET_CODE
else
    echo "ECS System compilation FAILED."
    exit 1
fi
    echo "ECS System compilation FAILED."
    exit 1
fi
