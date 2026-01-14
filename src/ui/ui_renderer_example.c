/*
 * ui_renderer_example.c
 * UI Rendering System Example
 * Demonstrates antialiasing, GPU rendering, and Z-ordering features
 * 
 * This example shows how to use the three implemented TODO features:
 * TODO-0204: Antialiasing for UI elements
 * TODO-0205: GPU-driven UI rendering  
 * TODO-0206: Z-order and depth sorting
 */

#include "ui_renderer.h"
#include <stdio.h>
#include <stdlib.h>

// Example UI element creation
static LayoutNode* create_example_ui_element(uint32_t id, const char* name, 
                                            float x, float y, float w, float h,
                                            int32_t z_index) {
    LayoutNode* element = malloc(sizeof(LayoutNode));
    if (!element) return NULL;
    
    element->id = id;
    element->name = name;
    element->parent = NULL;
    element->children = NULL;
    element->child_count = 0;
    element->child_capacity = 0;
    
    // Layout properties
    element->direction = LAYOUT_DIRECTION_ROW;
    element->align_items = ALIGN_START;
    element->align_self = ALIGN_START;
    element->align_content = ALIGN_START;
    element->justify_content = JUSTIFY_START;
    element->wrap = WRAP_NO_WRAP;
    element->position = POSITION_RELATIVE;
    
    // Flexbox properties
    element->flex_grow = 0.0f;
    element->flex_shrink = 1.0f;
    element->flex_basis = 0.0f;
    
    // Spacing
    element->margin[0] = element->margin[1] = element->margin[2] = element->margin[3] = 0.0f;
    element->padding[0] = element->padding[1] = element->padding[2] = element->padding[3] = 0.0f;
    element->border[0] = element->border[1] = element->border[2] = element->border[3] = 0.0f;
    
    // Size and computed layout
    element->width = w;
    element->height = h;
    element->computed_x = x;
    element->computed_y = y;
    element->computed_width = w;
    element->computed_height = h;
    element->z_index = z_index;
    
    // State
    element->is_dirty = false;
    element->needs_layout = false;
    element->is_visible = true;
    
    return element;
}

// Example vertex creation for a quad
static void create_quad_vertices(UIVertex* vertices, uint32_t* indices,
                               float x, float y, float w, float h,
                               uint32_t element_id, float z_depth) {
    // Positions and texture coordinates for a quad
    float positions[] = {
        x, y, 0.0f,      // Bottom-left
        x + w, y, 0.0f,  // Bottom-right
        x + w, y + h, 0.0f, // Top-right
        x, y + h, 0.0f   // Top-left
    };
    
    float texcoords[] = {
        0.0f, 1.0f,  // Bottom-left
        1.0f, 1.0f,  // Bottom-right
        1.0f, 0.0f,  // Top-right
        0.0f, 0.0f   // Top-left
    };
    
    // Create vertices
    for (int i = 0; i < 4; i++) {
        vertices[i].position[0] = positions[i * 3];
        vertices[i].position[1] = positions[i * 3 + 1];
        vertices[i].position[2] = positions[i * 3 + 2];
        
        vertices[i].texcoord[0] = texcoords[i * 2];
        vertices[i].texcoord[1] = texcoords[i * 2 + 1];
        
        vertices[i].color[0] = 1.0f;  // Red
        vertices[i].color[1] = 1.0f;  // Green
        vertices[i].color[2] = 1.0f;  // Blue
        vertices[i].color[3] = 1.0f;  // Alpha
        
        vertices[i].element_id = element_id;
        vertices[i].z_depth = z_depth;
    }
    
    // Create indices for two triangles
    indices[0] = 0; indices[1] = 1; indices[2] = 2;  // First triangle
    indices[3] = 0; indices[4] = 2; indices[5] = 3;  // Second triangle
}

// Demonstrate TODO-0204: Antialiasing for UI elements
static void demonstrate_antialiasing(UIRenderer* renderer) {
    printf("\\n=== Demonstrating TODO-0204: Antialiasing for UI Elements ===\\n");
    
    // Test different antialiasing modes
    UIAntialiasingMode aa_modes[] = {
        UI_AA_NONE,
        UI_AA_MSAA_2X,
        UI_AA_MSAA_4X,
        UI_AA_MSAA_8X,
        UI_AA_FXAA,
        UI_AA_TAA,
        UI_AA_SMAA
    };
    
    const char* aa_names[] = {
        "None",
        "MSAA 2X",
        "MSAA 4X", 
        "MSAA 8X",
        "FXAA",
        "TAA",
        "SMAA"
    };
    
    for (int i = 0; i < 7; i++) {
        printf("Setting antialiasing mode: %s\\n", aa_names[i]);
        ui_renderer_set_antialiasing_mode(renderer, aa_modes[i]);
        
        // Verify the mode was set
        UIAntialiasingMode current = ui_renderer_get_antialiasing_mode(renderer);
        printf("Current antialiasing mode: %s\\n", aa_names[current]);
    }
    
    printf("Antialiasing demonstration completed.\\n");
}

// Demonstrate TODO-0205: GPU-driven UI rendering
static void demonstrate_gpu_rendering(UIRenderer* renderer) {
    printf("\\n=== Demonstrating TODO-0205: GPU-Driven UI Rendering ===\\n");
    
    // Test different GPU backends
    UIGPUBackend backends[] = {
        UI_GPU_BACKEND_NONE,
        UI_GPU_BACKEND_OPENGL,
        UI_GPU_BACKEND_VULKAN,
        UI_GPU_BACKEND_METAL,
        UI_GPU_BACKEND_D3D11,
        UI_GPU_BACKEND_D3D12
    };
    
    const char* backend_names[] = {
        "None (Software)",
        "OpenGL",
        "Vulkan",
        "Metal",
        "Direct3D 11",
        "Direct3D 12"
    };
    
    for (int i = 0; i < 6; i++) {
        printf("Setting GPU backend: %s\\n", backend_names[i]);
        ui_renderer_set_gpu_backend(renderer, backends[i]);
        
        // Verify the backend was set
        UIGPUBackend current = ui_renderer_get_gpu_backend(renderer);
        printf("Current GPU backend: %s\\n", backend_names[current]);
        
        // Create a test element to render
        LayoutNode* element = create_example_ui_element(100 + i, "GPU_Test", 
                                                       50.0f + i * 20, 50.0f + i * 15, 
                                                       100.0f, 80.0f, i);
        if (element) {
            // Create vertices for the element
            UIVertex vertices[4];
            uint32_t indices[6];
            create_quad_vertices(vertices, indices, 
                               element->computed_x, element->computed_y,
                               element->computed_width, element->computed_height,
                               element->id, (float)i);
            
            // Submit element for GPU rendering
            ui_renderer_submit_element(renderer, element, vertices, 4, indices, 6, 0);
            
            free(element);
        }
    }
    
    // Flush to render all submitted elements
    ui_renderer_flush(renderer);
    
    printf("GPU rendering demonstration completed.\\n");
}

// Demonstrate TODO-0206: Z-order and depth sorting
static void demonstrate_z_ordering(UIRenderer* renderer) {
    printf("\\n=== Demonstrating TODO-0206: Z-Order and Depth Sorting ===\\n");
    
    // Test different Z-sorting modes
    UIZSortMode sort_modes[] = {
        UI_Z_SORT_NONE,
        UI_Z_SORT_PAINTERS,
        UI_Z_SORT_DEPTH_BUFFER,
        UI_Z_SORT_HYBRID,
        UI_Z_SORT_OPTIMIZED
    };
    
    const char* sort_names[] = {
        "None",
        "Painter's Algorithm",
        "Depth Buffer",
        "Hybrid",
        "Optimized"
    };
    
    for (int i = 0; i < 5; i++) {
        printf("Setting Z-sort mode: %s\\n", sort_names[i]);
        ui_renderer_set_z_sort_mode(renderer, sort_modes[i]);
        
        // Verify the mode was set
        UIZSortMode current = ui_renderer_get_z_sort_mode(renderer);
        printf("Current Z-sort mode: %s\\n", sort_names[current]);
        
        // Create overlapping elements with different Z-indexes to test sorting
        for (int j = 0; j < 5; j++) {
            LayoutNode* element = create_example_ui_element(200 + i * 10 + j, "Z_Test",
                                                           100.0f + j * 10, 100.0f + j * 10,
                                                           80.0f, 80.0f,
                                                           j - 2);  // Z-index from -2 to 2
            if (element) {
                // Create vertices with different colors based on Z-order
                UIVertex vertices[4];
                uint32_t indices[6];
                create_quad_vertices(vertices, indices,
                                   element->computed_x, element->computed_y,
                                   element->computed_width, element->computed_height,
                                   element->id, (float)(j - 2));
                
                // Set color based on Z-index (red = back, green = front)
                for (int k = 0; k < 4; k++) {
                    vertices[k].color[0] = (j == 0) ? 1.0f : 0.2f;  // Red for back
                    vertices[k].color[1] = (j == 4) ? 1.0f : 0.2f;  // Green for front
                    vertices[k].color[2] = 0.5f;  // Blue
                    vertices[k].color[3] = 0.8f;  // Alpha
                }
                
                // Submit element for Z-ordering test
                ui_renderer_submit_element(renderer, element, vertices, 4, indices, 6, 0);
                
                free(element);
            }
        }
        
        // Flush to render and test sorting
        ui_renderer_flush(renderer);
    }
    
    printf("Z-order and depth sorting demonstration completed.\\n");
}

// Main example function
int main() {
    printf("UI Rendering System Example\\n");
    printf("===========================\\n");
    printf("Demonstrating the three implemented TODO features:\\n");
    printf("- TODO-0204: Antialiasing for UI elements\\n");
    printf("- TODO-0205: GPU-driven UI rendering\\n");
    printf("- TODO-0206: Z-order and depth sorting\\n");
    
    // Initialize UI renderer with best settings
    UIRenderer renderer;
    bool success = ui_renderer_init(&renderer, 1920.0f, 1080.0f,
                                   UI_AA_MSAA_4X,        // Start with 4X MSAA
                                   UI_GPU_BACKEND_OPENGL, // Use OpenGL backend
                                   UI_Z_SORT_HYBRID);    // Use hybrid sorting
    
    if (!success) {
        printf("Failed to initialize UI renderer!\\n");
        return 1;
    }
    
    printf("\\nUI Renderer initialized successfully!\\n");
    printf("Viewport: 1920x1080\\n");
    printf("Antialiasing: MSAA 4X\\n");
    printf("GPU Backend: OpenGL\\n");
    printf("Z-Sorting: Hybrid\\n");
    
    // Demonstrate each feature
    demonstrate_antialiasing(&renderer);
    demonstrate_gpu_rendering(&renderer);
    demonstrate_z_ordering(&renderer);
    
    // Show final configuration
    printf("\\n=== Final Configuration ===\\n");
    printf("Antialiasing: %d\\n", ui_renderer_get_antialiasing_mode(&renderer));
    printf("GPU Backend: %d\\n", ui_renderer_get_gpu_backend(&renderer));
    printf("Z-Sort Mode: %d\\n", ui_renderer_get_z_sort_mode(&renderer));
    
    // Cleanup
    ui_renderer_shutdown(&renderer);
    
    printf("\\nUI Rendering System Example completed successfully!\\n");
    printf("All three TODO features have been demonstrated:\\n");
    printf(" TODO-0204: Antialiasing for UI elements - IMPLEMENTED\\n");
    printf(" TODO-0205: GPU-driven UI rendering - IMPLEMENTED\\n");
    printf(" TODO-0206: Z-order and depth sorting - IMPLEMENTED\\n");
    
    return 0;
}
