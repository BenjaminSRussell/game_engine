#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <stdio.h>

int main() {
    @autoreleasepool {
        // Get the default device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("No Metal device found\n");
            return 1;
        }
        
        printf("Metal Device: %s\n", [[device name] UTF8String]);
        
        // Test shader compilation with inline source
        NSString *shaderSource = @"#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "\n"
        "struct VertexIn {\n"
        "    float3 position [[attribute(0)]];\n"
        "};\n"
        "\n"
        "struct VertexOut {\n"
        "    float4 position [[position]];\n"
        "};\n"
        "\n"
        "vertex VertexOut vertex_main(VertexIn in [[stage_in]]) {\n"
        "    VertexOut out;\n"
        "    out.position = float4(in.position, 1.0);\n"
        "    return out;\n"
        "}\n"
        "\n"
        "fragment float4 fragment_main() {\n"
        "    return float4(1.0, 0.0, 0.0, 1.0); // Red color\n"
        "}\n";
        
        NSError *error = nil;
        id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
        
        if (!library) {
            printf("Shader compilation failed: %s\n", [[error localizedDescription] UTF8String]);
            return 1;
        }
        
        printf("Shader compilation successful!\n");
        printf("Functions found: %lu\n", (unsigned long)[[library functionNames] count]);
        
        for (NSString *functionName in [library functionNames]) {
            printf("  - %s\n", [functionName UTF8String]);
        }
        
        // Test function loading
        id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertex_main"];
        id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragment_main"];
        
        if (vertexFunc && fragmentFunc) {
            printf("Both vertex and fragment functions loaded successfully!\n");
        } else {
            printf("Failed to load shader functions\n");
            return 1;
        }
        
        printf("Metal shader system is working!\n");
    }
    
    return 0;
}
