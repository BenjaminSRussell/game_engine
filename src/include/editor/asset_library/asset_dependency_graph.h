#pragma once

#include <stdbool.h>

void asset_dep_graph_init(void);
void asset_dep_graph_add_node(const char *asset_path);
void asset_dep_graph_add_dependency(const char *dependent, const char *dependency);
void asset_dep_graph_remove_node(const char *asset_path);
bool asset_dep_graph_has_circular_dependency(const char *asset_path);
void asset_dep_graph_get_dependencies(const char *asset_path, void ***out_list, int *out_count);
