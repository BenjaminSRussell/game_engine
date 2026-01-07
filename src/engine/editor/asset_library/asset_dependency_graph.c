#include "editor/asset_library/asset_dependency_graph.h"

void asset_dep_graph_init() {}

void asset_dep_graph_add_dependency(const char *dependent, const char *dependency) {}

void asset_dep_graph_get_dependencies(const char *asset_path, void ***out_list, int *out_count) {}

void asset_dep_graph_find_references(const char *asset_path, void *output_list) {}
