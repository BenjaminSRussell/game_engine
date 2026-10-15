/*
 * mesh_primitives_test.c
 * Cube, sphere, cylinder, plane, cone, and torus generators.
 * Compiles mesh_primitives.c directly and does not link Engine.
 */

#include "geometry/mesh_primitives.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_failures++;
}

static int nearf(f32 a, f32 b) {
    return fabsf(a - b) <= 1.0e-3f;
}

static void release_mesh(mesh_t *mesh) {
    if (!mesh) {
        return;
    }
    free(mesh->vertices);
    free(mesh->indices);
    free(mesh->submeshes);
    free(mesh);
}

static void expect_null(mesh_t *mesh, const char *msg) {
    if (mesh != NULL) {
        fprintf(stderr, "FAIL: %s returned a mesh\n", msg);
        g_failures++;
        release_mesh(mesh);
    }
}

static f32 length3(f32 x, f32 y, f32 z) {
    return sqrtf(x * x + y * y + z * z);
}

static int axis_normal(Vec3 n) {
    int axes = 0;
    if (nearf(fabsf(n.x), 1.0f)) {
        axes++;
    }
    if (nearf(fabsf(n.y), 1.0f)) {
        axes++;
    }
    if (nearf(fabsf(n.z), 1.0f)) {
        axes++;
    }
    return axes == 1 && nearf(length3(n.x, n.y, n.z), 1.0f);
}

static void expect_layout(const mesh_t *mesh, const char *name, u32 vertices,
                          u32 indices, const char *label) {
    u32 i;

    if (!mesh || !mesh->vertices || !mesh->indices || !mesh->submeshes) {
        fprintf(stderr, "FAIL: %s missing mesh buffers\n", label);
        g_failures++;
        return;
    }
    if (mesh->vertex_count != vertices || mesh->index_count != indices) {
        fprintf(stderr, "FAIL: %s counts v=%u i=%u expected v=%u i=%u\n", label,
                mesh->vertex_count, mesh->index_count, vertices, indices);
        g_failures++;
    }
    if (strcmp(mesh->name, name) != 0) {
        fprintf(stderr, "FAIL: %s name '%s'\n", label, mesh->name);
        g_failures++;
    }
    if (mesh->submesh_count != 1 || mesh->submeshes[0].vertex_count != vertices ||
        mesh->submeshes[0].index_count != indices) {
        fprintf(stderr, "FAIL: %s submesh\n", label);
        g_failures++;
    }
    for (i = 0; i + 2 < mesh->index_count; i += 3) {
        u32 a = mesh->indices[i];
        u32 b = mesh->indices[i + 1];
        u32 c = mesh->indices[i + 2];
        if (a >= mesh->vertex_count || b >= mesh->vertex_count ||
            c >= mesh->vertex_count || a == b || b == c || a == c) {
            fprintf(stderr, "FAIL: %s triangle %u\n", label, i / 3);
            g_failures++;
            return;
        }
    }
    for (i = 0; i < mesh->vertex_count; i++) {
        Vec3 n = mesh->vertices[i].normal;
        if (!nearf(length3(n.x, n.y, n.z), 1.0f)) {
            fprintf(stderr, "FAIL: %s normal %u\n", label, i);
            g_failures++;
            return;
        }
    }
}

static void test_rejections(void) {
    expect_null(mesh_create_sphere(1.0f, 2), "sphere segments");
    expect_null(mesh_create_sphere(0.0f, 8), "sphere radius");
    expect_null(mesh_create_sphere(-1.0f, 8), "sphere negative radius");
    expect_null(mesh_create_cylinder(1.0f, 1.0f, 2), "cylinder segments");
    expect_null(mesh_create_cylinder(0.0f, 1.0f, 4), "cylinder radius");
    expect_null(mesh_create_cylinder(1.0f, 0.0f, 4), "cylinder height");
    expect_null(mesh_create_plane(0.0f, 1.0f, 1), "plane width");
    expect_null(mesh_create_plane(1.0f, -1.0f, 1), "plane depth");
    expect_null(mesh_create_plane(1.0f, 1.0f, 0), "plane subdivisions");
    expect_null(mesh_create_cone(1.0f, 1.0f, 2), "cone segments");
    expect_null(mesh_create_cone(-1.0f, 1.0f, 4), "cone radius");
    expect_null(mesh_create_cone(1.0f, 0.0f, 4), "cone height");
    expect_null(mesh_create_torus(1.0f, 0.25f, 2, 4), "torus major segments");
    expect_null(mesh_create_torus(1.0f, 0.25f, 4, 2), "torus minor segments");
    expect_null(mesh_create_torus(0.0f, 0.25f, 4, 4), "torus major radius");
    expect_null(mesh_create_torus(1.0f, -0.25f, 4, 4), "torus minor radius");
}

static void test_cube(void) {
    mesh_t *mesh = mesh_create_cube(2.0f);
    u32 i;
    int saw_corner = 0;

    expect_layout(mesh, "Cube", 24, 36, "cube");
    if (!mesh) {
        return;
    }
    if (mesh->indices[0] != 0 || mesh->indices[1] != 1 || mesh->indices[2] != 2 ||
        mesh->indices[3] != 0 || mesh->indices[4] != 2 || mesh->indices[5] != 3) {
        fail("cube first quad indices");
    }
    for (i = 0; i < mesh->vertex_count; i++) {
        Vec3 p = mesh->vertices[i].position;
        Vec2 uv = mesh->vertices[i].texcoord;
        if (!nearf(fabsf(p.x), 1.0f) || !nearf(fabsf(p.y), 1.0f) ||
            !nearf(fabsf(p.z), 1.0f)) {
            fail("cube corner extent");
            break;
        }
        if (!axis_normal(mesh->vertices[i].normal)) {
            fail("cube axis normal");
            break;
        }
        if (uv.x < -1.0e-4f || uv.x > 1.0f + 1.0e-4f || uv.y < -1.0e-4f ||
            uv.y > 1.0f + 1.0e-4f) {
            fail("cube texcoord");
            break;
        }
        if (nearf(p.x, -1.0f) && nearf(p.y, -1.0f) && nearf(p.z, 1.0f) &&
            nearf(mesh->vertices[i].normal.z, 1.0f)) {
            saw_corner = 1;
        }
    }
    if (!saw_corner) {
        fail("cube front corner");
    }
    release_mesh(mesh);
}

static void test_sphere(void) {
    const f32 radius = 2.0f;
    const u32 segments = 4;
    const u32 rings = segments;
    const u32 sectors = segments * 2;
    mesh_t *mesh = mesh_create_sphere(radius, segments);
    u32 i;

    expect_layout(mesh, "Sphere", (rings + 1) * (sectors + 1), rings * sectors * 6,
                  "sphere");
    if (!mesh) {
        return;
    }
    if (!nearf(mesh->vertices[0].position.x, 0.0f) ||
        !nearf(mesh->vertices[0].position.y, -radius) ||
        !nearf(mesh->vertices[0].position.z, 0.0f)) {
        fail("sphere south pole");
    }
    for (i = 0; i < mesh->vertex_count; i++) {
        Vec3 p = mesh->vertices[i].position;
        Vec3 n = mesh->vertices[i].normal;
        f32 len = length3(p.x, p.y, p.z);
        f32 dot = p.x * n.x + p.y * n.y + p.z * n.z;
        if (!nearf(len, radius) || !nearf(dot, radius)) {
            fail("sphere shell");
            break;
        }
    }
    release_mesh(mesh);
}

static void test_cylinder(void) {
    const u32 segments = 4;
    mesh_t *mesh = mesh_create_cylinder(1.0f, 2.0f, segments);
    u32 i;

    expect_layout(mesh, "Cylinder", segments * 2 + 2, segments * 12, "cylinder");
    if (!mesh) {
        return;
    }
    for (i = 0; i < segments * 2; i++) {
        Vec3 p = mesh->vertices[i].position;
        f32 radial = sqrtf(p.x * p.x + p.z * p.z);
        if (!nearf(radial, 1.0f) || !nearf(fabsf(p.y), 1.0f)) {
            fail("cylinder wall");
            break;
        }
    }
    if (!nearf(mesh->vertices[segments * 2].position.y, -1.0f) ||
        !nearf(mesh->vertices[segments * 2].normal.y, -1.0f) ||
        !nearf(mesh->vertices[segments * 2 + 1].position.y, 1.0f) ||
        !nearf(mesh->vertices[segments * 2 + 1].normal.y, 1.0f)) {
        fail("cylinder caps");
    }
    release_mesh(mesh);
}

static void test_plane(void) {
    mesh_t *mesh = mesh_create_plane(4.0f, 2.0f, 2);
    u32 i;

    expect_layout(mesh, "Plane", 9, 24, "plane");
    if (!mesh) {
        return;
    }
    for (i = 0; i < mesh->vertex_count; i++) {
        Vec3 p = mesh->vertices[i].position;
        Vec3 n = mesh->vertices[i].normal;
        if (!nearf(p.y, 0.0f) || p.x < -2.0f - 1.0e-3f || p.x > 2.0f + 1.0e-3f ||
            p.z < -1.0f - 1.0e-3f || p.z > 1.0f + 1.0e-3f || !nearf(n.x, 0.0f) ||
            !nearf(n.y, 1.0f) || !nearf(n.z, 0.0f)) {
            fail("plane surface");
            break;
        }
    }
    if (!nearf(mesh->vertices[0].position.x, -2.0f) ||
        !nearf(mesh->vertices[0].position.z, -1.0f) ||
        !nearf(mesh->vertices[8].position.x, 2.0f) ||
        !nearf(mesh->vertices[8].position.z, 1.0f)) {
        fail("plane corners");
    }
    release_mesh(mesh);
}

static void test_cone(void) {
    const u32 segments = 4;
    mesh_t *mesh = mesh_create_cone(1.0f, 3.0f, segments);
    u32 i;

    expect_layout(mesh, "Cone", segments + 2, segments * 6, "cone");
    if (!mesh) {
        return;
    }
    for (i = 0; i < segments; i++) {
        Vec3 p = mesh->vertices[i].position;
        f32 radial = sqrtf(p.x * p.x + p.z * p.z);
        if (!nearf(p.y, 0.0f) || !nearf(radial, 1.0f)) {
            fail("cone base ring");
            break;
        }
    }
    if (!nearf(mesh->vertices[segments].position.y, 3.0f) ||
        !nearf(mesh->vertices[segments].position.x, 0.0f) ||
        !nearf(mesh->vertices[segments].normal.y, 1.0f) ||
        !nearf(mesh->vertices[segments + 1].position.y, 0.0f) ||
        !nearf(mesh->vertices[segments + 1].normal.y, -1.0f)) {
        fail("cone tip and center");
    }
    release_mesh(mesh);
}

static void test_torus(void) {
    const f32 major = 2.0f;
    const f32 minor = 0.5f;
    const u32 major_segments = 4;
    const u32 minor_segments = 4;
    mesh_t *mesh = mesh_create_torus(major, minor, major_segments, minor_segments);
    u32 i;

    expect_layout(mesh, "Torus", (major_segments + 1) * (minor_segments + 1),
                  major_segments * minor_segments * 6, "torus");
    if (!mesh) {
        return;
    }
    for (i = 0; i < mesh->vertex_count; i++) {
        Vec3 p = mesh->vertices[i].position;
        f32 radial = sqrtf(p.x * p.x + p.z * p.z);
        f32 tube = sqrtf((radial - major) * (radial - major) + p.y * p.y);
        if (!nearf(tube, minor)) {
            fail("torus tube");
            break;
        }
    }
    release_mesh(mesh);
}

int main(void) {
    test_rejections();
    test_cube();
    test_sphere();
    test_cylinder();
    test_plane();
    test_cone();
    test_torus();
    if (g_failures != 0) {
        fprintf(stderr, "%d mesh primitive check(s) failed\n", g_failures);
        return 1;
    }
    printf("mesh primitives ok\n");
    return 0;
}
