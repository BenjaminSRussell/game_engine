#include "../editor_common.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                           MATERIAL COMPILER - GLSL GENERATOR
 * =================================================================================================
 * 
 * Provides functionality to traverse the Material Graph and generate GLSL shader code.
 * 
 */

// Helper to get variable name for a node's output
static void get_node_var_name(u32 node_id, u32 output_index, char* buffer, size_t size) {
    snprintf(buffer, size, "node_%u_out_%u", node_id, output_index);
}

// Convert connection type to GLSL type string
static const char* get_glsl_type(ConnectionType type) {
    switch (type) {
        case CONNECTION_TYPE_FLOAT: return "float";
        case CONNECTION_TYPE_VEC2: return "vec2";
        case CONNECTION_TYPE_VEC3: return "vec3";
        case CONNECTION_TYPE_VEC4: return "vec4";
        case CONNECTION_TYPE_TEXTURE: return "sampler2D";
        default: return "void";
    }
}

// Check if a node has been visited
static bool is_node_visited(u32 node_id, u32* visited_list, u32 visited_count) {
    for (u32 i = 0; i < visited_count; i++) {
        if (visited_list[i] == node_id) return true;
    }
    return false;
}

// Recursive function to generate code for a node and its dependencies
static bool generate_node_code(MaterialNode* node, char* code_buffer, size_t buffer_size, size_t* current_pos, 
                             u32* visited_list, u32* visited_count) {
    
    // If already generated, skip
    if (is_node_visited(node->node_id, visited_list, *visited_count)) {
        return true;
    }

    // Process dependencies first (inputs)
    for (u32 i = 0; i < node->input_count; i++) {
        if (node->inputs[i].connected_node) {
            if (!generate_node_code(node->inputs[i].connected_node, code_buffer, buffer_size, current_pos, visited_list, visited_count)) {
                return false;
            }
        }
    }

    // Generate code for this node
    char line[512];
    char input_vars[8][64]; // Temporary storage for input variable names

    // Resolve inputs
    for (u32 i = 0; i < node->input_count; i++) {
        if (node->inputs[i].connected_node) {
            get_node_var_name(node->inputs[i].connected_node->node_id, 
                            node->inputs[i].connected_output_index, 
                            input_vars[i], sizeof(input_vars[i]));
        } else {
            // Use default values if not connected
            switch (node->inputs[i].type) {
                case CONNECTION_TYPE_FLOAT: 
                    break;
                case CONNECTION_TYPE_VEC2: 
                    break;
                case CONNECTION_TYPE_VEC3: 
                    break;
                case CONNECTION_TYPE_VEC4: 
                    break;
                case CONNECTION_TYPE_TEXTURE: 
                    break;
                default: 
                    break;
            }
        }
    }
    
    // Generate operation logic
    switch (node->type) {
        case NODE_TYPE_CONSTANT_FLOAT:
            // Extract value from parameters
            if (node->parameters && node->parameter_size >= sizeof(f32)) {
                f32 value = *(f32*)node->parameters;
                snprintf(line, sizeof(line), "    float node_%u_out_0 = %.6f;\n", node->node_id, value);
            } else {
                // Fallback to default value if parameters not available
                snprintf(line, sizeof(line), "    float node_%u_out_0 = 1.0; // Default value\n", node->node_id);
            }
            break;
            
        case NODE_TYPE_CONSTANT_VEC3:
            // Extract value from parameters (should be 3 floats)
            if (node->parameters && node->parameter_size >= 3 * sizeof(f32)) {
                f32* vec3_data = (f32*)node->parameters;
                snprintf(line, sizeof(line), "    vec3 node_%u_out_0 = vec3(%.6f, %.6f, %.6f);\n", 
                        node->node_id, vec3_data[0], vec3_data[1], vec3_data[2]);
            } else {
                // Fallback to default white color if parameters not available
                snprintf(line, sizeof(line), "    vec3 node_%u_out_0 = vec3(1.0, 1.0, 1.0); // Default white\n", node->node_id);
            }
            break;

        case NODE_TYPE_TEXTURE_SAMPLE:
            // Check if UV input is connected, otherwise use default UVs
            char uv_input[64];
            if (node->input_count > 0 && node->inputs[0].connected_node) {
                get_node_var_name(node->inputs[0].connected_node->node_id, 
                                node->inputs[0].connected_output_index, 
                                uv_input, sizeof(uv_input));
            } else {
                snprintf(uv_input, sizeof(uv_input), "v_uv"); // Default UV coordinates
            }
            snprintf(line, sizeof(line), "    vec4 node_%u_out_0 = texture(u_texture_%u, %s);\n", 
                    node->node_id, node->node_id, uv_input);
            break;

        case NODE_TYPE_MULTIPLY:
             // Simplified type inference (assumes vec3 for now)
            snprintf(line, sizeof(line), "    vec3 node_%u_out_0 = %s * %s;\n", 
                    node->node_id, input_vars[0], input_vars[1]);
            break;

        case NODE_TYPE_ADD:
             snprintf(line, sizeof(line), "    vec3 node_%u_out_0 = %s + %s;\n", 
                    node->node_id, input_vars[0], input_vars[1]);
            break;

        case NODE_TYPE_PBR_MASTER:
            // Master node doesn't generate an output variable, it sets the final fragment globals
            snprintf(line, sizeof(line), 
                "    // PBR Master Output\n"
                "    vec3 albedo = %s;\n"
                "    vec3 normal = %s;\n"
                "    float metallic = %s;\n"
                "    float roughness = %s;\n"
                "    float ao = %s;\n",
                input_vars[0], input_vars[1], input_vars[2], input_vars[3], input_vars[4]);
            break;
            
        default:
            snprintf(line, sizeof(line), "    // Unknown node type %d\n", node->type);
            break;
    }

    // Append line to buffer
    size_t line_len = strlen(line);
    if (*current_pos + line_len < buffer_size) {
        strcpy(code_buffer + *current_pos, line);
        *current_pos += line_len;
    } else {
        LOG_ERROR("Shader code buffer invalid size");
        return false;
    }

    // Mark as visited
    visited_list[*visited_count] = node->node_id;
    (*visited_count)++;

    return true;
}

char* generate_fragment_shader(MaterialGraph* graph) {
    if (!graph || !graph->master_node) return NULL;

    size_t buffer_size = 16384; // 16KB buffer
    char* code = (char*)malloc(buffer_size);
    if (!code) return NULL;
    
    size_t pos = 0;

    // Header
    const char* header = 
        "#version 330 core\n\n"
        "in vec2 v_uv;\n"
        "in vec3 v_normal;\n"
        "in vec3 v_world_pos;\n\n"
        "out vec4 FragColor;\n\n"
        "// Uniforms would be generated here\n\n"
        "void main() {\n";
    
    strcpy(code, header);
    pos += strlen(header);

    // Visited list for topological sort
    u32* visited = (u32*)malloc(sizeof(u32) * graph->node_capacity);
    u32 visited_count = 0;

    // Generate code starting from Master Node dependencies
    // We don't generate the master node's "output" in the recursive call usually, 
    // but here we handled PBR Master specially in the switch.
    // However, the recursive function generates dependencies THEN the node itself.
    // So calling it on Master Node will generate everything ending with Master Node logic.
    
    if (!generate_node_code(graph->master_node, code, buffer_size, &pos, visited, &visited_count)) {
        free(code);
        free(visited);
        return NULL;
    }

    // Final PBR calculation (Placeholder)
    const char* footer = 
        "\n    // Simple PBR Placeholder\n"
        "    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));\n"
        "    float diff = max(dot(normal, lightDir), 0.0);\n"
        "    vec3 diffuse = albedo * diff;\n"
        "    FragColor = vec4(diffuse, 1.0);\n"
        "}\n";
        
    if (pos + strlen(footer) < buffer_size) {
        strcpy(code + pos, footer);
    }

    free(visited);
    return code;
}

bool material_compiler_compile(MaterialGraph* graph) {
    if (!graph->master_node) {
        LOG_ERROR("Cannot compile: No master node");
        return false;
    }

    // Clean up old shader
    if (graph->generated_fragment_shader) {
        free(graph->generated_fragment_shader);
        graph->generated_fragment_shader = NULL;
    }

    // Generate new shader
    graph->generated_fragment_shader = generate_fragment_shader(graph);
    
    if (graph->generated_fragment_shader) {
        LOG_INFO("Shader generated successfully (%zu bytes)", strlen(graph->generated_fragment_shader));
        return true;
    } else {
        LOG_ERROR("Failed to generate shader");
        return false;
    }
}
