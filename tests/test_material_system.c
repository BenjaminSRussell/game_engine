#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "materials/material_system.h"
#include "core/logger.h"

// Mock logger to avoid linking full engine in test
void log_message(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

int main() {
    printf("=== Test Material System ===\n");
    
    // 1. Initialize
    if (!material_system_init()) {
        printf("Failed to init material system\n");
        return 1;
    }
    printf("[Pass] System initialized\n");
    
    // 2. Check Defaults
    MaterialMaster *pbr = material_system_get_master("M_PBR_Standard");
    if (!pbr) {
        printf("Failed to get default PBR master\n");
        return 1;
    }
    printf("[Pass] Found default PBR master\n");
    
    // 3. Create Instance
    MaterialInstance *grass = material_system_create_instance("M_PBR_Standard", "M_Grass_High");
    if (!grass) {
        printf("Failed to create grass instance\n");
        return 1;
    }
    printf("[Pass] Created grass instance\n");
    
    // 4. Override Parameter
    // BaseColor default is White, set to Green
    f32 green[3] = {0.0f, 1.0f, 0.0f};
    material_instance_set_vec3(grass, "BaseColor", green);
    
    // Verify override
    MaterialParameter *param = material_instance_get_param(grass, "BaseColor");
    if (param->value.vec3_val[1] != 1.0f) {
        printf("Failed to override BaseColor\n");
        return 1;
    }
    printf("[Pass] Parameter override working\n");
    
    // 5. Lookup Instance
    MaterialInstance *lookup = material_system_get_instance("M_Grass_High");
    if (lookup != grass) {
        printf("Failed to lookup instance by name\n");
        return 1;
    }
    printf("[Pass] Instance lookup working\n");
    
    // 6. Shutdown
    material_system_shutdown();
    printf("[Pass] Shutdown complete\n");
    
    printf("=== All Tests Passed ===\n");
    return 0;
}
