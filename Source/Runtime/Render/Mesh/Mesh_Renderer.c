#include "../Public/Render.h"
#include <stdlib.h>

// Implementation of Mesh Renderer

// Redefinition of struct from internal scope (if needed) or just usage of
// opaque pointer. Public header has typedef.

MeshRenderer *MeshRenderer_Create(void) {
  return malloc(1); // Stub
}

void MeshRenderer_Destroy(MeshRenderer *renderer) { free(renderer); }

// Stubs for other renderers to avoid link errors for now
SpriteRenderer *SpriteRenderer_Create(void) { return malloc(1); }
void SpriteRenderer_Destroy(SpriteRenderer *renderer) { free(renderer); }

UIRenderer *UIRenderer_Create(void) { return malloc(1); }
void UIRenderer_Destroy(UIRenderer *renderer) { free(renderer); }
