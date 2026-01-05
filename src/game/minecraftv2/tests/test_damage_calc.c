#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "combat/damage_calc.h>

int main(void) {
    /* Test: full durability diamond armor vs physical damage */
    f32 damage = 10.0f;
    f32 mitigated = damage_apply_armor(damage, ARMOR_TYPE_DIAMOND, 1.0f, DAMAGE_TYPE_PHYSICAL);
    /* Diamond should reduce ~20% physical damage, so result ~8.0 */
    assert(mitigated < damage);
    assert(mitigated > 5.0f);

    /* Test: damaged armor vs physical damage */
    f32 mitigated_damaged = damage_apply_armor(damage, ARMOR_TYPE_DIAMOND, 0.3f, DAMAGE_TYPE_MAGIC);
    /* Magic damage with degraded diamond should be less effective */
    assert(mitigated_damaged > mitigated);

    /* Test: no armor */
    f32 no_armor = damage_apply_armor(damage, ARMOR_TYPE_NONE, 1.0f, DAMAGE_TYPE_PHYSICAL);
    assert(no_armor == damage);

    printf("test_damage_calc: OK (%.1f -> %.1f vs diamond full, %.1f vs diamond damaged)\n",
           damage, mitigated, mitigated_damaged);
    return 0;
}
