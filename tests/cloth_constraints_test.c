/*
 * cloth_constraints_test.c
 * Minimal harness for cloth constraints lifecycle + async flow.
 */

#include "character/cloth/simulation_cloth/cloth_constraints.h"
#include <stdio.h>
#include <string.h>

static int run_cloth_constraints_smoke(void) {
    if (cloth_system_cloth_constraints_init() != 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    cloth_system_cloth_constraints_handle_t handle = {0};
    cloth_system_cloth_constraints_desc_t desc = {0};
    desc.flags = (1u << 3) | (1u << 5); /* validation + async */

    if (cloth_system_cloth_constraints_create(&handle, &desc) != 0) {
        fprintf(stderr, "create failed\n");
        cloth_system_cloth_constraints_shutdown();
        return 2;
    }

    const char payload[] = "constraint_blob_v1";
    if (cloth_system_cloth_constraints_update(handle, payload, sizeof(payload)) != 0) {
        fprintf(stderr, "update failed\n");
        cloth_system_cloth_constraints_destroy(handle);
        cloth_system_cloth_constraints_shutdown();
        return 3;
    }

    if (cloth_system_cloth_constraints_process_pending() <= 0) {
        fprintf(stderr, "process_pending did not process anything\n");
        cloth_system_cloth_constraints_destroy(handle);
        cloth_system_cloth_constraints_shutdown();
        return 4;
    }

    /* Re-send identical payload to hit cache path. */
    if (cloth_system_cloth_constraints_update(handle, payload, sizeof(payload)) != 0) {
        fprintf(stderr, "second update failed\n");
        cloth_system_cloth_constraints_destroy(handle);
        cloth_system_cloth_constraints_shutdown();
        return 5;
    }

    cloth_system_cloth_constraints_info_t info = {0};
    if (cloth_system_cloth_constraints_get_info(handle, &info) != 0 || !info.initialized) {
        fprintf(stderr, "info invalid\n");
        cloth_system_cloth_constraints_destroy(handle);
        cloth_system_cloth_constraints_shutdown();
        return 6;
    }

    cloth_system_cloth_constraints_debug_print();
    cloth_system_cloth_constraints_destroy(handle);
    cloth_system_cloth_constraints_shutdown();
    return 0;
}

int main(void) {
    int result = run_cloth_constraints_smoke();
    if (result == 0) {
        printf("cloth_constraints_test: ok\n");
    }
    return result;
}
