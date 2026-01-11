#include <assert.h>
#include <stdio.h>
#include <physics/physics.h>
#include <physics/physics_engine_core.h>

void test_unique_ids() {
    printf("Testing Unique IDs...\n");
    RigidBody *b1 = rigid_body_create(BODY_TYPE_DYNAMIC, (Vec3){0, 0, 0});
    RigidBody *b2 = rigid_body_create(BODY_TYPE_DYNAMIC, (Vec3){1, 0, 0});
    RigidBody *b3 = rigid_body_create(BODY_TYPE_STATIC, (Vec3){0, -1, 0});

    assert(b1 != NULL);
    assert(b2 != NULL);
    assert(b3 != NULL);

    printf("Body 1 ID: %u\n", b1->id);
    printf("Body 2 ID: %u\n", b2->id);
    printf("Body 3 ID: %u\n", b3->id);

    assert(b1->id != 0);
    assert(b2->id != 0);
    assert(b3->id != 0);

    assert(b1->id != b2->id);
    assert(b1->id != b3->id);
    assert(b2->id != b3->id);

    // Assuming sequential generation for this test, but uniqueness is the key requirement.
    // If it's sequential starting from 1 (or whatever current state is), b2 > b1, b3 > b2.
    // But we don't strictly require order, just uniqueness.

    rigid_body_destroy(b1);
    rigid_body_destroy(b2);
    rigid_body_destroy(b3);

    printf("Unique IDs Test Passed\n");
}

int main() {
    test_unique_ids();
    return 0;
}
