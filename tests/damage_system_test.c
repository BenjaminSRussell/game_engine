/*
 * damage_system_test.c
 * Resistance, armor, the event queue, and health application.
 * Compiles damage_system.c directly and stubs ECS so it does not link Engine.
 */

#include "ecs/component_ids.h"
#include "ecs/components/health.h"
#include "ecs/components/transform.h"
#include "gameplay/combat/damage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* damage_system.c replaces the component-id enum with this stub value. */
enum { kDamageSystemResistanceId = 50 };

#define MAX_ID 8

static HealthComponent *g_health[MAX_ID];
static TransformComponent *g_transform[MAX_ID];
static ResistanceComponent *g_resistance[MAX_ID];
static int g_destroy_calls;
static Entity g_destroyed;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect(int cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static int near(f32 a, f32 b) {
    f32 d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.001f;
}

static Entity make_entity(u32 id) {
    Entity entity;
    entity.id = id;
    entity.generation = 1;
    return entity;
}

static void reset_world(void) {
    memset(g_health, 0, sizeof(g_health));
    memset(g_transform, 0, sizeof(g_transform));
    memset(g_resistance, 0, sizeof(g_resistance));
    g_destroy_calls = 0;
    g_destroyed = make_entity(0);
}

void *ecs_get_component(World *world, Entity entity, ECSComponentID component_id) {
    (void)world;
    if (entity.id >= MAX_ID) {
        return NULL;
    }
    if (component_id == HEALTH_COMPONENT_ID) {
        return g_health[entity.id];
    }
    if (component_id == TRANSFORM_COMPONENT_ID) {
        return g_transform[entity.id];
    }
    if (component_id == kDamageSystemResistanceId) {
        return g_resistance[entity.id];
    }
    return NULL;
}

void ecs_destroy_entity(World *world, Entity entity) {
    (void)world;
    g_destroy_calls++;
    g_destroyed = entity;
}

static DamageEvent event_of(Entity source, Entity target, f32 amount, DamageType type) {
    DamageEvent event;
    memset(&event, 0, sizeof(event));
    event.source = source;
    event.target = target;
    event.base_amount = amount;
    event.final_amount = amount;
    event.type = type;
    return event;
}

static void expect_damage(const DamageEvent *event, const ResistanceComponent *resistance,
                          f32 expected_final, f32 expected_blocked, const char *label) {
    f32 blocked = -1.0f;
    f32 final_damage = damage_calculate_final(event, resistance, &blocked);
    printf("%s -> final=%.2f blocked=%.2f\n", label, final_damage, blocked);
    expect(near(final_damage, expected_final), label);
    expect(near(blocked, expected_blocked), label);
}

static void test_calculate(void) {
    Entity source = make_entity(1);
    Entity target = make_entity(2);
    DamageEvent physical = event_of(source, target, 100.0f, DAMAGE_TYPE_PHYSICAL);
    DamageEvent fire = event_of(source, target, 100.0f, DAMAGE_TYPE_FIRE);
    DamageEvent truth = event_of(source, target, 40.0f, DAMAGE_TYPE_TRUE);
    DamageEvent negative = event_of(source, target, -5.0f, DAMAGE_TYPE_FIRE);
    DamageEvent unknown = event_of(source, target, 12.0f, DAMAGE_TYPE_COUNT);
    ResistanceComponent resistance;
    f32 ignored = 7.0f;

    expect(damage_calculate_final(NULL, NULL, &ignored) == 0.0f, "null event");
    expect(near(ignored, 7.0f), "null event leaves blocked alone");
    expect(near(damage_calculate_final(&physical, NULL, NULL), 100.0f), "null blocked out");

    expect_damage(&physical, NULL, 100.0f, 0.0f, "no resistance");
    expect_damage(&negative, NULL, 0.0f, 0.0f, "negative base clamps to zero");

    memset(&resistance, 0, sizeof(resistance));
    resistance.resistances[DAMAGE_TYPE_FIRE] = 0.25f;
    resistance.resistances[DAMAGE_TYPE_PHYSICAL] = 0.20f;
    resistance.armor = 30.0f;
    expect_damage(&fire, &resistance, 75.0f, 25.0f, "fire resistance");
    expect_damage(&physical, &resistance, 50.0f, 50.0f, "physical resistance then armor");

    resistance.armor = 100.0f;
    resistance.resistances[DAMAGE_TYPE_PHYSICAL] = 0.0f;
    physical.base_amount = 8.0f;
    expect_damage(&physical, &resistance, 0.0f, 8.0f, "armor cannot exceed remaining");

    resistance.armor = 50.0f;
    expect_damage(&truth, &resistance, 40.0f, 0.0f, "armor does not apply to true damage");
    expect_damage(&unknown, &resistance, 12.0f, 0.0f, "out-of-range type skips the table");
}

static void test_helpers(void) {
    expect(strcmp(damage_type_to_string(DAMAGE_TYPE_PHYSICAL), "Physical") == 0, "physical name");
    expect(strcmp(damage_type_to_string(DAMAGE_TYPE_FIRE), "Fire") == 0, "fire name");
    expect(strcmp(damage_type_to_string(DAMAGE_TYPE_TRUE), "True") == 0, "true name");
    expect(strcmp(damage_type_to_string(DAMAGE_TYPE_COUNT), "Unknown") == 0, "unknown name");
    expect(damage_can_crit(DAMAGE_TYPE_PHYSICAL), "physical can crit");
    expect(damage_can_crit(DAMAGE_TYPE_MAGIC), "magic can crit");
    expect(!damage_can_crit(DAMAGE_TYPE_FIRE), "fire cannot crit");
    expect(!damage_can_crit(DAMAGE_TYPE_TRUE), "true cannot crit");
    printf("helpers ok\n");
}

static void test_queue_and_apply(void) {
    static World world;
    Entity source = make_entity(1);
    Entity healthy = make_entity(2);
    Entity bare = make_entity(3);
    Entity dying = make_entity(4);
    HealthComponent health;
    HealthComponent dying_health;
    TransformComponent transform;
    ResistanceComponent resistance;
    DamageEvent *created;
    DamageEvent *second;
    DamageEvent *third;
    int i;

    damage_system_shutdown();
    damage_system_process_events(&world, 1.0);
    reset_world();

    damage_system_init(3);
    created = damage_event_create(source, healthy, 12.0f, DAMAGE_TYPE_FIRE);
    expect(created != NULL, "create");
    expect(created->source.id == source.id, "source id");
    expect(created->target.id == healthy.id, "target id");
    expect(near(created->base_amount, 12.0f), "base amount");
    expect(near(created->final_amount, 12.0f), "final starts as base");
    expect(created->type == DAMAGE_TYPE_FIRE, "created type");
    expect(created->timestamp == 0.0, "timestamp placeholder");

    damage_event_emit(NULL);
    damage_event_emit(created);
    second = damage_event_create(source, healthy, 3.0f, DAMAGE_TYPE_PHYSICAL);
    third = damage_event_create(source, bare, 9.0f, DAMAGE_TYPE_PHYSICAL);
    expect(second != NULL && third != NULL, "emit does not consume a slot");
    expect(damage_event_create(source, healthy, 1.0f, DAMAGE_TYPE_PHYSICAL) == NULL, "queue full");

    damage_system_clear_events();
    memset(&health, 0, sizeof(health));
    health.health = 50.0f;
    health.max_health = 50.0f;
    health.is_alive = true;
    g_health[healthy.id] = &health;
    damage_event_emit_simple(source, healthy, 7.0f);
    damage_system_clear_events();
    damage_system_process_events(&world, 0.25);
    expect(near(health.health, 50.0f), "clear drops the event");
    expect(g_destroy_calls == 0, "clear does not destroy");

    damage_event_create(source, healthy, 7.0f, DAMAGE_TYPE_FIRE);
    damage_system_process_events(NULL, 0.5);
    expect(near(health.health, 50.0f), "null world does not apply");
    damage_system_process_events(&world, 1.5);
    expect(near(health.health, 43.0f), "queued event applies after a null world");
    expect(near(health.last_damage_time, 1.5f), "last damage time is the frame dt");
    expect(health.is_alive, "still alive");
    expect(g_destroy_calls == 0, "no death");
    damage_system_process_events(&world, 2.0);
    expect(near(health.health, 43.0f), "process clears the queue");

    memset(&resistance, 0, sizeof(resistance));
    resistance.resistances[DAMAGE_TYPE_PHYSICAL] = 0.5f;
    resistance.armor = 5.0f;
    g_resistance[healthy.id] = &resistance;
    health.health = 100.0f;
    damage_event_create(source, bare, 40.0f, DAMAGE_TYPE_PHYSICAL);
    damage_event_create(source, healthy, 40.0f, DAMAGE_TYPE_PHYSICAL);
    damage_system_process_events(&world, 0.1);
    expect(near(health.health, 85.0f), "resistance and armor during process");
    expect(g_destroy_calls == 0, "missing health is skipped");
    g_resistance[healthy.id] = NULL;

    memset(&dying_health, 0, sizeof(dying_health));
    dying_health.health = 10.0f;
    dying_health.max_health = 10.0f;
    dying_health.is_alive = true;
    g_health[dying.id] = &dying_health;
    memset(&transform, 0, sizeof(transform));
    transform.position = vec3(4.0f, 5.0f, 6.0f);
    g_transform[dying.id] = &transform;
    damage_event_create(source, dying, 4.0f, DAMAGE_TYPE_MAGIC);
    damage_event_create(source, dying, 8.0f, DAMAGE_TYPE_MAGIC);
    damage_system_process_events(&world, 3.0);
    expect(near(dying_health.health, 0.0f), "lethal damage clamps at zero");
    expect(!dying_health.is_alive, "death clears alive");
    expect(g_destroy_calls == 1, "destroy once");
    expect(g_destroyed.id == dying.id, "destroyed the target");

    g_transform[dying.id] = NULL;
    dying_health.health = 1.0f;
    dying_health.is_alive = true;
    g_destroy_calls = 0;
    damage_event_create(source, dying, 2.0f, DAMAGE_TYPE_DARK);
    damage_system_process_events(&world, 1.0);
    expect(g_destroy_calls == 1, "death without a transform still destroys");
    expect(!dying_health.is_alive, "dead without a transform");

    damage_system_shutdown();
    expect(damage_event_create(source, healthy, 1.0f, DAMAGE_TYPE_PHYSICAL) == NULL,
           "create after shutdown");
    damage_system_process_events(&world, 1.0);

    damage_system_init(0);
    for (i = 0; i < 1024; i++) {
        if (!damage_event_create(source, healthy, 1.0f, DAMAGE_TYPE_TRUE)) {
            fail("default capacity is 1024");
        }
    }
    expect(damage_event_create(source, healthy, 1.0f, DAMAGE_TYPE_TRUE) == NULL,
           "1025th event is dropped");
    damage_system_shutdown();
    printf("queue and apply ok\n");
}

int main(void) {
    test_calculate();
    test_helpers();
    test_queue_and_apply();
    printf("damage_system_test: ok\n");
    return 0;
}
