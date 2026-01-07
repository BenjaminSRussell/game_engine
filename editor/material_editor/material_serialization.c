#include "../editor_common.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                           MATERIAL SERIALIZATION
 * =================================================================================================
 * 
 * Handles saving and loading Material Graphs to disk.
 * Format: Custom Text/JSON-like format for simplicity.
 */

// -------------------------------------------------------------------------------------------------
// WRITER
// -------------------------------------------------------------------------------------------------

bool material_graph_save(MaterialGraph* graph, const char* filepath) {
    if (!graph || !filepath) return false;
    
    FILE* file = fopen(filepath, "w");
    if (!file) {
        LOG_ERROR("Failed to open file for writing: %s", filepath);
        return false;
    }
    
    // Header
    fprintf(file, "MATERIAL_GRAPH v%u\n", graph->version);
    fprintf(file, "NAME \"%s\"\n", graph->name);
    fprintf(file, "AUTHOR \"%s\"\n", graph->author);
    
    // Nodes
    fprintf(file, "NODES %u\n", graph->node_count);
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* node = &graph->nodes[i];
        fprintf(file, "NODE %u %d \"%s\" %.2f %.2f\n", 
            node->node_id, 
            node->type, 
            node->name, 
            node->ui_position.x, 
            node->ui_position.y);
            
        // Save parameters if any (simplified)
        // In a real impl, we'd serialize based on node type
    }
    
    // Connections
    fprintf(file, "CONNECTIONS\n");
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* dest_node = &graph->nodes[i];
        for (u32 j = 0; j < dest_node->input_count; j++) {
            if (dest_node->inputs[j].connected_node) {
                fprintf(file, "LINK %u %u -> %u %u\n",
                    dest_node->inputs[j].connected_node->node_id,
                    dest_node->inputs[j].connected_output_index,
                    dest_node->node_id,
                    j);
            }
        }
    }
    
    fclose(file);
    LOG_INFO("Saved material graph to %s", filepath);
    return true;
}

// -------------------------------------------------------------------------------------------------
// READER
// -------------------------------------------------------------------------------------------------

bool material_graph_load(MaterialGraph* graph, const char* filepath) {
    if (!graph || !filepath) return false;
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        LOG_ERROR("Failed to open file for reading: %s", filepath);
        return false;
    }
    
    char line[256];
    char temp_str[128];
    
    // Reset graph
    // (Assuming graph is already initialized but empty, or caller handles clear)
    
    while (fgets(line, sizeof(line), file)) {
        // Basic parsing logic
        if (strncmp(line, "NAME", 4) == 0) {
            sscanf(line, "NAME \"%[^\"]\"", graph->name);
        } else if (strncmp(line, "AUTHOR", 6) == 0) {
            sscanf(line, "AUTHOR \"%[^\"]\"", graph->author);
        } else if (strncmp(line, "NODE", 4) == 0) {
            u32 id;
            int type;
            f32 x, y;
            sscanf(line, "NODE %u %d \"%[^\"]\" %f %f", &id, &type, temp_str, &x, &y);
            
            MaterialNode* node = material_graph_add_node(graph, (MaterialNodeType)type);
            if (node) {
                node->node_id = id; // Override ID to match file
                strncpy(node->name, temp_str, sizeof(node->name));
                node->ui_position.x = x;
                node->ui_position.y = y;
            }
        } else if (strncmp(line, "LINK", 4) == 0) {
            u32 src_id, src_out, dest_id, dest_in;
            sscanf(line, "LINK %u %u -> %u %u", &src_id, &src_out, &dest_id, &dest_in);
            material_graph_connect_nodes(graph, src_id, src_out, dest_id, dest_in);
        }
    }
    
    fclose(file);
    LOG_INFO("Loaded material graph from %s", filepath);
    return true;
}
