#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../include/vfx/particle_system.h>
#include "../include/crafting/brewing.h>
#include "../include/crafting/enchanting.h>
#include "../include/math/vec3.h>

int main(void) {
    ParticleSystem ps;
    particle_system_init(&ps, NULL);

    /* Test brewing VFX */
    BrewingStand stand = {0};
    stand.position = (Vec3){10.0f, 64.0f, 10.0f};

    u32 before = particle_get_active_count(&ps);
    brewing_emit_complete_vfx(&stand, &ps, NULL);
    u32 after = particle_get_active_count(&ps);

    if (after <= before) {
        printf("FAIL: brewing_emit_complete_vfx didn't emit particles\n");
        return 1;
    }

    /* Test brewing progress VFX */
    before = particle_get_active_count(&ps);
    stand.brew_progress = 0.5f;
    brewing_emit_progress_vfx(&stand, &ps);
    after = particle_get_active_count(&ps);

    if (after <= before) {
        printf("FAIL: brewing_emit_progress_vfx didn't emit particles\n");
        return 1;
    }

    /* Test enchanting VFX */
    Enchantment ench = {0};
    ench.type = 1;
    Vec3 pos = {20.0f, 65.0f, 20.0f};

    before = particle_get_active_count(&ps);
    enchanting_emit_apply_vfx(&ench, pos, &ps, NULL);
    after = particle_get_active_count(&ps);

    if (after <= before) {
        printf("FAIL: enchanting_emit_apply_vfx didn't emit particles\n");
        return 1;
    }

    printf("test_vfx: OK\n");
    return 0;
}
