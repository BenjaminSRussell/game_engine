#pragma once

#include <stdbool.h>

void world_partition_init(void);
void world_partition_update(void);
void world_partition_scan(void);
void world_partition_load_cell(int x, int z);
void world_partition_unload_cell(int x, int z);
bool world_partition_is_loaded(int x, int z);
