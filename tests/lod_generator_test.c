/*
 * lod_generator_test.c
 * LOD edge collapse must remap deleted vertices onto the survivor,
 * honor the caller's ratio count, and apply generation config weights.
 * Compiles lod_generator.c and mesh.c directly and does not link Engine.
 */

#include "geometry/lod/lod_generator.h"
#include "geometry/mesh.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(bool cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static geometry_vertex_t vtx(float x, float y, float z, float nx, float ny,
                             float nz, float u, float v) {
    geometry_vertex_t vert;
    memset(&vert, 0, sizeof(vert));
    vert.position = vec3_create(x, y, z);
    vert.normal = vec3_create(nx, ny, nz);
    vert.texcoord = vec2_create(u, v);
    return vert;
}

static mesh_t *make_mesh(const geometry_vertex_t *verts, u32 vcount,
                         const u32 *indices, u32 icount) {
    mesh_t *mesh = mesh_create("lod_src");
    if (!mesh) {
        fail("mesh_create");
    }
    mesh_allocate_buffers(mesh, vcount, icount);
    mesh_set_vertices(mesh, verts, vcount, 0);
    if (icount > 0) {
        mesh_set_indices(mesh, indices, icount, 0);
    }
    return mesh;
}

static int near_pos(Vec3 p, float x, float y, float z, float eps) {
    return fabsf(p.x - x) <= eps && fabsf(p.y - y) <= eps &&
           fabsf(p.z - z) <= eps;
}

static int has_position(const mesh_t *mesh, float x, float y, float z) {
    u32 i;
    for (i = 0; i < mesh->vertex_count; i++) {
        if (near_pos(mesh->vertices[i].position, x, y, z, 1e-3f)) {
            return 1;
        }
    }
    return 0;
}

static int triangle_uses_position(const mesh_t *mesh, float x, float y,
                                  float z) {
    u32 i;
    for (i = 0; i + 2 < mesh->index_count; i += 3) {
        int corner;
        for (corner = 0; corner < 3; corner++) {
            u32 idx = mesh->indices[i + (u32)corner];
            if (idx >= mesh->vertex_count) {
                fail("index out of range");
            }
            if (near_pos(mesh->vertices[idx].position, x, y, z, 1e-3f)) {
                return 1;
            }
        }
    }
    return 0;
}

static void expect_clean_mesh(const mesh_t *mesh, const char *msg) {
    u32 i;
    expect_true(mesh_validate(mesh), msg);
    expect_true(mesh->index_count % 3 == 0, "index count multiple of 3");
    for (i = 0; i + 2 < mesh->index_count; i += 3) {
        u32 a = mesh->indices[i];
        u32 b = mesh->indices[i + 1];
        u32 c = mesh->indices[i + 2];
        expect_true(a != b && b != c && a != c, "collapsed corner survived");
    }
}

static mesh_t *make_strip(void) {
    geometry_vertex_t verts[6];
    u32 indices[12];
    submesh_t sub;

    verts[0] = vtx(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    verts[1] = vtx(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    verts[2] = vtx(2.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    verts[3] = vtx(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    verts[4] = vtx(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f);
    verts[5] = vtx(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

    indices[0] = 0;
    indices[1] = 3;
    indices[2] = 1;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 4;
    indices[6] = 1;
    indices[7] = 4;
    indices[8] = 2;
    indices[9] = 2;
    indices[10] = 4;
    indices[11] = 5;

    mesh_t *mesh = make_mesh(verts, 6, indices, 12);
    memset(&sub, 0, sizeof(sub));
    sub.index_start = 0;
    sub.index_count = 12;
    sub.vertex_start = 0;
    sub.vertex_count = 6;
    sub.material_index = 0;
    mesh_add_submesh(mesh, sub);
    mesh_set_material(mesh, 0, 7);
    return mesh;
}

static mesh_t *make_two_islands(float short_len, float long_len, float bnx,
                                float bny, float bnz, float bu, float bv) {
    geometry_vertex_t verts[6];
    u32 indices[6];

    verts[0] = vtx(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    verts[1] = vtx(short_len, 0.0f, 0.0f, bnx, bny, bnz, bu, bv);
    verts[2] = vtx(0.0f, 3.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    verts[3] = vtx(10.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    verts[4] = vtx(10.0f + long_len, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    verts[5] = vtx(10.0f, 3.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    indices[4] = 4;
    indices[5] = 5;
    return make_mesh(verts, 6, indices, 6);
}

static lod_generation_config_t open_config(void) {
    lod_generation_config_t config = lod_get_default_config();
    config.preserve_boundaries = false;
    config.preserve_seams = false;
    config.preserve_features = false;
    config.quality_weights[0] = 1.0f;
    config.quality_weights[1] = 0.0f;
    config.quality_weights[2] = 0.0f;
    config.normal_smoothing = 1.0f;
    return config;
}

static void test_null_and_range(void) {
    mesh_t *strip = make_strip();
    f32 ratios[] = {0.5f};
    f32 err = 42.0f;
    lod_generation_config_t config = lod_get_default_config();

    expect_true(config.reduction_ratios[0] == 0.75f, "default ratio 0");
    expect_true(config.reduction_ratios[3] == 0.1f, "default ratio 3");
    expect_true(config.preserve_boundaries, "default preserve boundaries");
    expect_true(config.quality_weights[0] == 1.0f, "default position weight");

    expect_true(lod_generate_chain(NULL, ratios, 1) == NULL, "null source chain");
    expect_true(lod_generate_chain(strip, NULL, 1) == NULL, "null ratios");
    expect_true(lod_generate_chain(strip, ratios, 0) == NULL, "zero ratio count");
    expect_true(lod_generate_chain_advanced(NULL, &config) == NULL,
                "null advanced source");
    expect_true(qem_simplify(NULL, 0.5f) == NULL, "null simplify");
    expect_true(qem_simplify(strip, 0.0f) == NULL, "zero ratio");
    expect_true(qem_simplify(strip, 1.0f) == NULL, "ratio one");
    expect_true(qem_simplify(strip, -0.2f) == NULL, "negative ratio");
    expect_true(qem_simplify(strip, 1.5f) == NULL, "ratio above one");
    expect_true(qem_simplify_advanced(NULL, 0.5f, &config, &err) == NULL,
                "null advanced simplify");
    expect_true(err == 0.0f, "error cleared on failure");
    lod_chain_destroy(NULL);

    mesh_destroy(strip);
}

static void test_collapse_remaps_incident_triangle(void) {
    mesh_t *strip = make_strip();
    mesh_t *lod;
    f32 err = -1.0f;
    u32 source_indices = strip->index_count;
    lod_generation_config_t config = lod_get_default_config();

    lod = qem_simplify_advanced(strip, 0.8f, &config, &err);
    expect_true(lod != NULL, "simplify strip");
    expect_true(strip->index_count == source_indices, "source not mutated");
    expect_true(lod->index_count / 3 == 2, "one collapse removes two triangles");
    expect_true(lod->vertex_count == 5, "one vertex removed");
    expect_true(has_position(lod, 1.0f, 0.5f, 0.0f), "survivor moved to midpoint");
    expect_true(triangle_uses_position(lod, 2.0f, 0.0f, 0.0f),
                "triangle across the collapsed vertex was remapped");
    expect_true(!triangle_uses_position(lod, 1.0f, 0.0f, 0.0f),
                "deleted endpoint is not referenced");
    expect_true(err > 0.15f && err < 0.18f, "vertex reduction error");
    expect_true(lod->material_count == 1 && lod->material_ids[0] == 7,
                "material copied");
    expect_true(lod->submesh_count == 1, "submesh copied");
    expect_true(lod->submeshes[0].index_start == 0 &&
                    lod->submeshes[0].index_count == lod->index_count,
                "submesh covers simplified indices");
    expect_true(lod->bounds.max.x >= 1.9f, "bounds include far vertex");
    expect_clean_mesh(lod, "simplified strip validates");

    mesh_destroy(lod);
    mesh_destroy(strip);
}

static void test_chain_uses_only_requested_ratios(void) {
    mesh_t *strip = make_strip();
    f32 one[] = {0.8f};
    f32 two[] = {0.8f, 0.5f};
    f32 skipped[] = {0.8f, 0.0f, 2.0f};
    lod_chain_t *chain;
    lod_chain_t *defaults;

    chain = lod_generate_chain(strip, one, 1);
    expect_true(chain != NULL, "chain one ratio");
    expect_true(chain->lod_count == 2, "no leftover default LODs");
    expect_true(chain->lods[0] == strip, "LOD0 is the source mesh");
    expect_true(chain->geometric_errors[0] == 0.0f, "LOD0 error");
    expect_true(chain->switch_distances[0] == 1.0f, "LOD0 distance");
    expect_true(chain->switch_distances[1] == 0.8f, "requested switch distance");
    expect_true(chain->lods[1] != NULL, "LOD1 mesh");
    expect_true(triangle_uses_position(chain->lods[1], 2.0f, 0.0f, 0.0f),
                "LOD1 kept the far corner");
    expect_true(chain->geometric_errors[1] > 0.0f, "LOD1 error");
    lod_chain_destroy(chain);
    expect_true(strip->index_count == 12, "destroy leaves source");

    chain = lod_generate_chain(strip, two, 2);
    expect_true(chain != NULL && chain->lod_count == 3, "two requested LODs");
    lod_chain_destroy(chain);

    chain = lod_generate_chain(strip, skipped, 3);
    expect_true(chain != NULL && chain->lod_count == 2, "invalid ratios skipped");
    lod_chain_destroy(chain);

    defaults = lod_generate_chain_advanced(strip, NULL);
    expect_true(defaults != NULL && defaults->lod_count == 5,
                "null config uses four default ratios");
    lod_chain_destroy(defaults);
    mesh_destroy(strip);
}

static void test_quality_weights_and_features(void) {
    mesh_t *islands;
    mesh_t *lod;
    lod_generation_config_t config = open_config();
    f32 err = -1.0f;

    config.quality_weights[0] = 1.0f;
    config.quality_weights[1] = 0.0f;
    islands = make_two_islands(0.05f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    lod = qem_simplify_advanced(islands, 0.5f, &config, &err);
    expect_true(lod != NULL, "distance-weighted simplify");
    expect_true(!has_position(lod, 0.05f, 0.0f, 0.0f),
                "shortest edge collapses when position weight dominates");
    expect_true(has_position(lod, 10.5f, 0.0f, 0.0f),
                "longer edge survives distance weighting");
    expect_true(err > 0.0f, "weight simplify error");
    mesh_destroy(lod);
    mesh_destroy(islands);

    config.quality_weights[1] = 10.0f;
    islands = make_two_islands(0.05f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    lod = qem_simplify_advanced(islands, 0.5f, &config, NULL);
    expect_true(lod != NULL, "normal-weighted simplify");
    expect_true(has_position(lod, 0.05f, 0.0f, 0.0f),
                "sharp short edge loses when normal weight is high");
    expect_true(has_position(lod, 10.25f, 0.0f, 0.0f),
                "smoother edge collapses under normal weight");
    expect_true(!has_position(lod, 10.5f, 0.0f, 0.0f),
                "longer endpoint removed");
    mesh_destroy(lod);
    mesh_destroy(islands);

    config = open_config();
    config.preserve_features = true;
    config.feature_angle = 30.0f;
    islands = make_two_islands(0.05f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    lod = qem_simplify_advanced(islands, 0.5f, &config, NULL);
    expect_true(lod != NULL, "feature-preserving simplify");
    expect_true(has_position(lod, 0.05f, 0.0f, 0.0f),
                "sharp edge kept");
    expect_true(!has_position(lod, 10.5f, 0.0f, 0.0f),
                "non-feature edge collapsed");
    mesh_destroy(lod);
    mesh_destroy(islands);
}

static void test_seams_and_normal_smoothing(void) {
    mesh_t *islands;
    mesh_t *lod;
    lod_generation_config_t config = open_config();
    int found = 0;
    u32 i;

    config.preserve_seams = true;
    config.weld_distance = 0.001f;
    islands = make_two_islands(0.0001f, 0.4f, 0.0f, 1.0f, 0.0f, 0.8f, 0.2f);
    lod = qem_simplify_advanced(islands, 0.5f, &config, NULL);
    expect_true(lod != NULL, "seam simplify");
    expect_true(has_position(lod, 0.0001f, 0.0f, 0.0f),
                "UV seam duplicate was not welded");
    expect_true(has_position(lod, 10.2f, 0.0f, 0.0f),
                "non-seam edge collapsed instead");
    expect_true(!has_position(lod, 10.4f, 0.0f, 0.0f),
                "non-seam endpoint removed");
    mesh_destroy(lod);
    mesh_destroy(islands);

    config = open_config();
    config.normal_smoothing = 0.0f;
    islands = make_two_islands(0.1f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    lod = qem_simplify_advanced(islands, 0.5f, &config, NULL);
    expect_true(lod != NULL, "smooth-zero simplify");
    for (i = 0; i < lod->vertex_count; i++) {
        if (near_pos(lod->vertices[i].position, 0.05f, 0.0f, 0.0f, 1e-3f)) {
            expect_true(near_pos(lod->vertices[i].normal, 0.0f, 1.0f, 0.0f, 1e-3f),
                        "smoothing 0 keeps the survivor normal");
            found = 1;
        }
    }
    expect_true(found, "smoothed survivor present");
    mesh_destroy(lod);
    mesh_destroy(islands);

    found = 0;
    config.normal_smoothing = 1.0f;
    islands = make_two_islands(0.1f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    lod = qem_simplify_advanced(islands, 0.5f, &config, NULL);
    expect_true(lod != NULL, "smooth-one simplify");
    for (i = 0; i < lod->vertex_count; i++) {
        if (near_pos(lod->vertices[i].position, 0.05f, 0.0f, 0.0f, 1e-3f)) {
            float ny = lod->vertices[i].normal.y;
            float nz = lod->vertices[i].normal.z;
            expect_true(fabsf(ny - 0.70710678f) < 1e-3f &&
                            fabsf(nz - 0.70710678f) < 1e-3f,
                        "smoothing 1 averages normals");
            found = 1;
        }
    }
    expect_true(found, "averaged survivor present");
    mesh_destroy(lod);
    mesh_destroy(islands);
}

static void test_out_of_range_index_is_skipped(void) {
    geometry_vertex_t verts[1];
    u32 indices[3];
    mesh_t *mesh;
    mesh_t *lod;

    verts[0] = vtx(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    indices[0] = 9;
    indices[1] = 9;
    indices[2] = 9;
    mesh = make_mesh(verts, 1, indices, 3);
    lod = qem_simplify(mesh, 0.5f);
    expect_true(lod != NULL, "bad index simplify");
    expect_true(lod->index_count == 0, "out of range triangle dropped");
    expect_true(lod->vertex_count == 1, "vertex kept");
    expect_clean_mesh(lod, "bad index result validates");
    mesh_destroy(lod);
    mesh_destroy(mesh);
}

int main(void) {
    test_null_and_range();
    test_collapse_remaps_incident_triangle();
    test_chain_uses_only_requested_ratios();
    test_quality_weights_and_features();
    test_seams_and_normal_smoothing();
    test_out_of_range_index_is_skipped();
    printf("lod_generator_test ok\n");
    return 0;
}
