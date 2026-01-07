/*
 * lightmap_packer.c
 * Lightmap UV Packer (Rectangle Packing)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/lightmaps/lightmap_packer.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_ATLAS_SIZE 4096

typedef struct node {
    struct node* child[2];
    int x, y, w, h;
    bool occupied;
} node_t;

static node_t* root = NULL;

static node_t* create_node(int x, int y, int w, int h) {
    node_t* n = malloc(sizeof(node_t));
    n->x = x; n->y = y; n->w = w; n->h = h;
    n->child[0] = NULL; n->child[1] = NULL;
    n->occupied = false;
    return n;
}

static node_t* insert_rect(node_t* node, int w, int h) {
    if (node->child[0] != NULL) {
        node_t* new_node = insert_rect(node->child[0], w, h);
        if (new_node != NULL) return new_node;
        return insert_rect(node->child[1], w, h);
    }

    if (node->occupied) return NULL;
    if (node->w < w || node->h < h) return NULL;

    if (node->w == w && node->h == h) {
        node->occupied = true;
        return node;
    }

    // Split
    node->child[0] = create_node(node->x, node->y, w, h); // This logic is simplified; usually split by free space
    // Proper splitting:
    int dw = node->w - w;
    int dh = node->h - h;

    if (dw > dh) {
        node->child[0] = create_node(node->x, node->y, w, node->h);
        node->child[1] = create_node(node->x + w, node->y, dw, node->h);
    } else {
        node->child[0] = create_node(node->x, node->y, node->w, h);
        node->child[1] = create_node(node->x, node->y + h, node->w, dh);
    }
    
    // Recurse into the first child which is now the perfect size (or still larger)
    return insert_rect(node->child[0], w, h);
}

void lightmap_packer_init(int width, int height) {
    root = create_node(0, 0, width, height);
}

bool lightmap_packer_pack(int w, int h, int* out_x, int* out_y) {
    node_t* n = insert_rect(root, w, h);
    if (n) {
        *out_x = n->x;
        *out_y = n->y;
        return true;
    }
    return false;
}

void lightmap_packer_shutdown(void) {
    // recursively free nodes
}
