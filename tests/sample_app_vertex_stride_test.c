#include "sample_vertex_layout.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#include <simd/simd.h>
#endif

#ifndef SAMPLE_APP_M
#error SAMPLE_APP_M must be the path to src/sample_app.m
#endif

static int g_failures = 0;

static void expect(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        g_failures++;
    }
}

/* 1 if an uncommented line contains needle. */
static int uncommented_contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "r");
    char line[2048];
    int found = 0;

    if (!f) {
        fprintf(stderr, "FAIL: cannot open %s\n", path);
        return -1;
    }
    while (fgets(line, sizeof line, f)) {
        const char *p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '/' && p[1] == '/') {
            continue;
        }
        if (strstr(p, needle)) {
            found = 1;
        }
    }
    fclose(f);
    return found;
}

#ifdef __APPLE__
typedef struct {
    vector_float3 position;
    vector_float3 color;
    vector_float3 normal;
    vector_float2 uv;
    vector_float4 tangent;
} SampleVertexAbi;

_Static_assert(sizeof(SampleVertexAbi) == SAMPLE_VERTEX_STRIDE,
               "descriptor stride must match the simd Vertex size");
_Static_assert(offsetof(SampleVertexAbi, position) == SAMPLE_VERTEX_POSITION_OFFSET, "position");
_Static_assert(offsetof(SampleVertexAbi, color) == SAMPLE_VERTEX_COLOR_OFFSET, "color");
_Static_assert(offsetof(SampleVertexAbi, normal) == SAMPLE_VERTEX_NORMAL_OFFSET, "normal");
_Static_assert(offsetof(SampleVertexAbi, uv) == SAMPLE_VERTEX_UV_OFFSET, "uv");
_Static_assert(offsetof(SampleVertexAbi, tangent) == SAMPLE_VERTEX_TANGENT_OFFSET, "tangent");
#endif

int main(void) {
    size_t n;
    int listed;

    expect(sample_vertex_attribute_end() <= (size_t)SAMPLE_VERTEX_STRIDE,
           "attributes must lie inside the descriptor stride");
    expect(sample_vertex_reads_fit(0), "empty buffer has no reads");

    for (n = 1; n <= 8; n++) {
        size_t packed_length = n * 44u;

        expect(sample_vertex_reads_fit(n), "matching stride must keep the last vertex in range");
        expect(sample_vertex_buffer_length(n) == n * (size_t)SAMPLE_VERTEX_STRIDE,
               "buffer length is vertex count times the descriptor stride");
        expect(sample_vertex_last_read_end(n) > packed_length,
               "the old 44-byte SimpleVertex buffer walks off the descriptor");
    }

    listed = uncommented_contains(SAMPLE_APP_M, "sample_vertex_layout.h");
    expect(listed == 1, "sample_app.m must use the shared vertex layout");
    listed = uncommented_contains(SAMPLE_APP_M, "typedef Vertex SimpleVertex");
    expect(listed == 1, "SimpleVertex must be the descriptor Vertex");
    listed = uncommented_contains(SAMPLE_APP_M, "vector_float4 tangent");
    expect(listed == 1, "Vertex must carry the tangent the descriptor fetches");
    listed = uncommented_contains(SAMPLE_APP_M, "layouts[0].stride = SAMPLE_VERTEX_STRIDE");
    expect(listed == 1, "descriptor stride must be the shared stride");
    listed = uncommented_contains(SAMPLE_APP_M, "sample_vertex_buffer_length");
    expect(listed == 1, "vertex buffer length must use the descriptor stride");
    listed = uncommented_contains(SAMPLE_APP_M, "SAMPLE_VERTEX_TANGENT_OFFSET");
    expect(listed == 1, "tangent fetch offset must be the shared offset");
    listed = uncommented_contains(SAMPLE_APP_M,
                                 "_Static_assert(sizeof(SimpleVertex) == SAMPLE_VERTEX_STRIDE");
    expect(listed == 1, "SimpleVertex size must be asserted against the descriptor stride");
    listed = uncommented_contains(SAMPLE_APP_M, "sizeof(SimpleVertex) *");
    expect(listed == 0, "allocation must not use a separate SimpleVertex size");
    listed = uncommented_contains(SAMPLE_APP_M, "* sizeof(SimpleVertex)");
    expect(listed == 0, "upload must not size the buffer as packed SimpleVertex");

    return g_failures == 0 ? 0 : 1;
}
