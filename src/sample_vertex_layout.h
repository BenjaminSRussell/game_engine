#ifndef SAMPLE_VERTEX_LAYOUT_H
#define SAMPLE_VERTEX_LAYOUT_H

#include <stddef.h>

/* Stride shared by the sample app's Metal vertex descriptor and its vertex
 * buffer. Apple vector_float3 is 16 bytes and vector_float4 is 16-byte
 * aligned, so Vertex is 80 bytes with tangent at offset 64. A packed
 * position/color/normal/uv record is 44 bytes and is read past the buffer.
 */
enum {
    SAMPLE_VERTEX_POSITION_OFFSET = 0,
    SAMPLE_VERTEX_COLOR_OFFSET = 16,
    SAMPLE_VERTEX_NORMAL_OFFSET = 32,
    SAMPLE_VERTEX_UV_OFFSET = 48,
    SAMPLE_VERTEX_TANGENT_OFFSET = 64,
    SAMPLE_VERTEX_POSITION_SIZE = 12,
    SAMPLE_VERTEX_COLOR_SIZE = 12,
    SAMPLE_VERTEX_NORMAL_SIZE = 12,
    SAMPLE_VERTEX_UV_SIZE = 8,
    SAMPLE_VERTEX_TANGENT_SIZE = 16,
    SAMPLE_VERTEX_STRIDE = 80
};

static inline size_t sample_vertex_attribute_end(void) {
    size_t ends[5];
    size_t end;
    size_t i;

    ends[0] = (size_t)SAMPLE_VERTEX_POSITION_OFFSET + (size_t)SAMPLE_VERTEX_POSITION_SIZE;
    ends[1] = (size_t)SAMPLE_VERTEX_COLOR_OFFSET + (size_t)SAMPLE_VERTEX_COLOR_SIZE;
    ends[2] = (size_t)SAMPLE_VERTEX_NORMAL_OFFSET + (size_t)SAMPLE_VERTEX_NORMAL_SIZE;
    ends[3] = (size_t)SAMPLE_VERTEX_UV_OFFSET + (size_t)SAMPLE_VERTEX_UV_SIZE;
    ends[4] = (size_t)SAMPLE_VERTEX_TANGENT_OFFSET + (size_t)SAMPLE_VERTEX_TANGENT_SIZE;
    end = 0;
    for (i = 0; i < 5; i++) {
        if (ends[i] > end) {
            end = ends[i];
        }
    }
    return end;
}

static inline size_t sample_vertex_buffer_length(size_t vertex_count) {
    return vertex_count * (size_t)SAMPLE_VERTEX_STRIDE;
}

/* Exclusive end offset of the last attribute read for vertex_count records. */
static inline size_t sample_vertex_last_read_end(size_t vertex_count) {
    if (vertex_count == 0) {
        return 0;
    }
    return (vertex_count - 1) * (size_t)SAMPLE_VERTEX_STRIDE + sample_vertex_attribute_end();
}

/* Non-zero when every attribute fetch stays inside the uploaded buffer. */
static inline int sample_vertex_reads_fit(size_t vertex_count) {
    if (sample_vertex_attribute_end() > (size_t)SAMPLE_VERTEX_STRIDE) {
        return 0;
    }
    return sample_vertex_last_read_end(vertex_count) <= sample_vertex_buffer_length(vertex_count);
}

#endif
