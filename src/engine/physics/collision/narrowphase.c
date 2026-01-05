#include "physics/collision/narrowphase.h"

void narrowphase_init() {}

int narrowphase_test_sphere_sphere(void *a, void *b, void *contact) {
    return 0;
}

int narrowphase_test_box_box(void *a, void *b, void *contact) {
    return 0;
}

int narrowphase_test_sphere_box(void *sphere, void *box, void *contact) {
    return 0;
}

int narrowphase_test_mesh_mesh(void *a, void *b, void *contacts) {
    return 0;
}
