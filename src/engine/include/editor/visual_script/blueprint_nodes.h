#pragma once

// Basic types for blueprint nodes
typedef struct BlueprintNode BlueprintNode;
typedef struct BlueprintPin BlueprintPin;

void blueprint_register_nodes(void);
void blueprint_node_create(const char *type, float x, float y);
void blueprint_node_connect(void *node_a, const char *pin_a, void *node_b, const char *pin_b);
