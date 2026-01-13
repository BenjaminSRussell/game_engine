// Physics Demos Main Entry Point
#include "physics_demo_framework.c"
#include <core/logger.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    LOG_INFO("Starting Physics Demos");
    
    // Initialize the demo framework
    if (!physics_demo_framework_init()) {
        LOG_ERROR("Failed to initialize physics demo framework");
        return 1;
    }
    
    // Run the demo framework
    physics_demo_framework_run();
    
    // Cleanup
    physics_demo_framework_cleanup();
    
    LOG_INFO("Physics Demos finished");
    return 0;
}
