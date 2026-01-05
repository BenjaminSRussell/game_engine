#pragma once

#include <stdbool.h>

void asset_collection_create(const char *name);
void asset_collection_delete(const char *name);
void asset_collection_add_asset(const char *collection, const char *asset_path);
void asset_collection_remove_asset(const char *collection, const char *asset_path);
void asset_collection_rename(const char *old_name, const char *new_name);
