#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../include/vfx/particle_system.h>
#include "../include/crafting/brewing.h>
#include "../include/chunk/chunk.h>
#include "../include/math/vec3.h>

int main(void) {
    ParticleSystem ps;
    particle_system_init(&ps, NULL);

    /* Setup chunk manager */
    ChunkManager mgr = {0};
    chunk_manager_init(&mgr, 4);

    /* Create and register a brewing stand */
    BrewingStand stand = {0};
    stand.position = (Vec3){10.0f, 64.0f, 10.0f};
    brewing_start_brew(&stand, 0, BREW_INGREDIENT_NETHER_WART, 1);
    chunk_manager_register_brewing_stand(&mgr, &stand);

    u32 before = particle_get_active_count(&ps);
    chunk_manager_update(&mgr, 0.1f, &ps, NULL);
    u32 after = particle_get_active_count(&ps);

    if (after <= before) {
        printf("FAIL: chunk_manager_update didn't cause brewing VFX to emit\n");
        return 1;
    }

    printf("test_chunk_brewing_vfx: OK\n");
    return 0;
}
