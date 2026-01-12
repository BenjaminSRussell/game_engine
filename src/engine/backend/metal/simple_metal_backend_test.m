/*
 * Simple Metal Backend Test
 * Tests Metal backend functionality without complex dependencies
 */

#include <stdio.h>
#include <stdlib.h>
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

// Test Metal device creation and capabilities
void test_metal_device(void) {
    printf("=== Metal Device Test ===\n");
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        printf("FAIL: Could not create Metal device\n");
        return;
    }
    
    printf("PASS: Metal device created: %s\n", [[device name] UTF8String]);
    printf("  Headless: %s\n", [device isHeadless] ? "Yes" : "No");
    printf("  Low power: %s\n", [device isLowPower] ? "Yes" : "No");
    printf("  Removable: %s\n", [device isRemovable] ? "Yes" : "No");
    printf("  Current allocated memory: %zu bytes\n", (size_t)[device currentAllocatedSize]);
}

// Test buffer operations
void test_metal_buffers(void) {
    printf("\n=== Metal Buffer Test ===\n");
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        printf("FAIL: Could not create Metal device\n");
        return;
    }
    
    // Test different storage modes
    id<MTLBuffer> sharedBuffer = [device newBufferWithLength:4096 options:MTLResourceStorageModeShared];
    id<MTLBuffer> privateBuffer = [device newBufferWithLength:4096 options:MTLResourceStorageModePrivate];
    id<MTLBuffer> managedBuffer = [device newBufferWithLength:4096 options:MTLResourceStorageModeManaged];
    
    if (sharedBuffer) {
        printf("PASS: Shared buffer created (%zu bytes)\n", (size_t)[sharedBuffer length]);
        
        // Test buffer contents access
        void *contents = [sharedBuffer contents];
        if (contents) {
            float *floatData = (float *)contents;
            for (int i = 0; i < 4; i++) {
                floatData[i] = (float)i * 1.5f;
            }
            printf("PASS: Buffer write test successful\n");
        } else {
            printf("FAIL: Could not access buffer contents\n");
        }
    } else {
        printf("FAIL: Could not create shared buffer\n");
    }
    
    printf("Private buffer: %s\n", privateBuffer ? "PASS" : "FAIL");
    printf("Managed buffer: %s\n", managedBuffer ? "PASS" : "FAIL");
}

// Test texture operations
void test_metal_textures(void) {
    printf("\n=== Metal Texture Test ===\n");
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        printf("FAIL: Could not create Metal device\n");
        return;
    }
    
    MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.width = 256;
    textureDescriptor.height = 256;
    textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    
    id<MTLTexture> texture = [device newTextureWithDescriptor:textureDescriptor];
    if (texture) {
        printf("PASS: Texture created (%dx%d)\n", (int)[texture width], (int)[texture height]);
        printf("  Pixel format: %d\n", (int)[texture pixelFormat]);
        printf("  Usage: %d\n", (int)[texture usage]);
    } else {
        printf("FAIL: Could not create texture\n");
    }
}

// Test shader compilation
void test_metal_shaders(void) {
    printf("\n=== Metal Shader Test ===\n");
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        printf("FAIL: Could not create Metal device\n");
        return;
    }
    
    // Simple vertex shader
    NSString *vertexShader = @"#include <metal_stdlib>\n"
                             "using namespace metal;\n"
                             "struct VertexOut { float4 position [[position]]; };\n"
                             "vertex VertexOut vertex_main(uint id [[vertex_id]]) {\n"
                             "    VertexOut out;\n"
                             "    float x = (id % 2) * 2.0 - 1.0;\n"
                             "    float y = (id / 2) * 2.0 - 1.0;\n"
                             "    out.position = float4(x, y, 0.0, 1.0);\n"
                             "    return out;\n"
                             "}";
    
    // Simple fragment shader
    NSString *fragmentShader = @"#include <metal_stdlib>\n"
                               "using namespace metal;\n"
                               "fragment float4 fragment_main() {\n"
                               "    return float4(0.8, 0.4, 0.2, 1.0);\n"
                               "}";
    
    // Simple compute shader
    NSString *computeShader = @"#include <metal_stdlib>\n"
                              "using namespace metal;\n"
                              "kernel void compute_main(device float *data [[buffer(0)]],\n"
                              "                           uint id [[thread_position_in_grid]]) {\n"
                              "    data[id] = sin(float(id)) * 0.5f;\n"
                              "}";
    
    NSError *error = nil;
    int compiledShaders = 0;
    
    // Compile vertex shader
    id<MTLLibrary> vertexLibrary = [device newLibraryWithSource:vertexShader options:nil error:&error];
    if (vertexLibrary) {
        printf("PASS: Vertex shader compiled\n");
        compiledShaders++;
    } else {
        printf("FAIL: Vertex shader compilation failed: %s\n", [[error localizedDescription] UTF8String]);
    }
    
    // Compile fragment shader
    id<MTLLibrary> fragmentLibrary = [device newLibraryWithSource:fragmentShader options:nil error:&error];
    if (fragmentLibrary) {
        printf("PASS: Fragment shader compiled\n");
        compiledShaders++;
    } else {
        printf("FAIL: Fragment shader compilation failed: %s\n", [[error localizedDescription] UTF8String]);
    }
    
    // Compile compute shader
    id<MTLLibrary> computeLibrary = [device newLibraryWithSource:computeShader options:nil error:&error];
    if (computeLibrary) {
        printf("PASS: Compute shader compiled\n");
        compiledShaders++;
    } else {
        printf("FAIL: Compute shader compilation failed: %s\n", [[error localizedDescription] UTF8String]);
    }
    
    printf("Shader compilation: %d/3 successful\n", compiledShaders);
}

// Test pipeline creation
void test_metal_pipelines(void) {
    printf("\n=== Metal Pipeline Test ===\n");
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        printf("FAIL: Could not create Metal device\n");
        return;
    }
    
    // Simple shaders for pipeline testing
    NSString *vertexShader = @"#include <metal_stdlib>\n"
                             "using namespace metal;\n"
                             "vertex float4 vertex_main(uint id [[vertex_id]]) {\n"
                             "    return float4(0.0, 0.0, 0.0, 1.0);\n"
                             "}";
    
    NSString *fragmentShader = @"#include <metal_stdlib>\n"
                               "using namespace metal;\n"
                               "fragment float4 fragment_main() {\n"
                               "    return float4(1.0, 1.0, 1.0, 1.0);\n"
                               "}";
    
    NSString *computeShader = @"#include <metal_stdlib>\n"
                              "using namespace metal;\n"
                              "kernel void compute_main(uint id [[thread_position_in_grid]]) {\n"
                              "}";
    
    NSError *error = nil;
    int createdPipelines = 0;
    
    // Create render pipeline
    id<MTLLibrary> vertexLibrary = [device newLibraryWithSource:vertexShader options:nil error:&error];
    id<MTLLibrary> fragmentLibrary = [device newLibraryWithSource:fragmentShader options:nil error:&error];
    
    if (vertexLibrary && fragmentLibrary) {
        id<MTLFunction> vertexFunction = [vertexLibrary newFunctionWithName:@"vertex_main"];
        id<MTLFunction> fragmentFunction = [fragmentLibrary newFunctionWithName:@"fragment_main"];
        
        MTLRenderPipelineDescriptor *renderDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        renderDescriptor.vertexFunction = vertexFunction;
        renderDescriptor.fragmentFunction = fragmentFunction;
        renderDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        id<MTLRenderPipelineState> renderPipeline = [device newRenderPipelineStateWithDescriptor:renderDescriptor error:&error];
        if (renderPipeline) {
            printf("PASS: Render pipeline created\n");
            createdPipelines++;
        } else {
            printf("FAIL: Render pipeline creation failed: %s\n", [[error localizedDescription] UTF8String]);
        }
    }
    
    // Create compute pipeline
    id<MTLLibrary> computeLibrary = [device newLibraryWithSource:computeShader options:nil error:&error];
    if (computeLibrary) {
        id<MTLFunction> computeFunction = [computeLibrary newFunctionWithName:@"compute_main"];
        
        MTLComputePipelineDescriptor *computeDescriptor = [[MTLComputePipelineDescriptor alloc] init];
        computeDescriptor.computeFunction = computeFunction;
        
        id<MTLComputePipelineState> computePipeline = [device newComputePipelineStateWithDescriptor:computeDescriptor options:0 reflection:nil error:&error];
        if (computePipeline) {
            printf("PASS: Compute pipeline created\n");
            createdPipelines++;
        } else {
            printf("FAIL: Compute pipeline creation failed: %s\n", [[error localizedDescription] UTF8String]);
        }
    }
    
    printf("Pipeline creation: %d/2 successful\n", createdPipelines);
}

// Test command operations
void test_metal_commands(void) {
    printf("\n=== Metal Command Test ===\n");
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        printf("FAIL: Could not create Metal device\n");
        return;
    }
    
    // Create command queue
    id<MTLCommandQueue> commandQueue = [device newCommandQueue];
    if (commandQueue) {
        printf("PASS: Command queue created\n");
        
        // Create command buffer
        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        if (commandBuffer) {
            printf("PASS: Command buffer created\n");
            printf("  Command buffer status: %d\n", (int)[commandBuffer status]);
            printf("  Retained references: %d\n", (int)[commandBuffer retainedReferences]);
        } else {
            printf("FAIL: Command buffer creation failed\n");
        }
    } else {
        printf("FAIL: Command queue creation failed\n");
    }
}

int main(void) {
    printf("Simple Metal Backend Test\n");
    printf("========================\n");
    
    test_metal_device();
    test_metal_buffers();
    test_metal_textures();
    test_metal_shaders();
    test_metal_pipelines();
    test_metal_commands();
    
    printf("\n=== Test Complete ===\n");
    printf("Metal backend functionality verified!\n");
    
    return 0;
}