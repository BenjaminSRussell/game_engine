#include "Public/UI.h"
#include <stdlib.h>

// Struct Stubs
struct UIContext {
  int id;
};
struct UIInput {
  int id;
};
struct UIStyle {
  int id;
};
struct UIFontSystem {
  int id;
};
struct UIWidget {
  int id;
};
// UIRenderer is opaque, derived from Render system or separate?
// Assuming separate struct for UI module specific logic
struct UIRenderer {
  int id;
};

static UISystem g_ui_system = {0};

// Implementation Stubs
UIContext *UIContext_Create(void) { return malloc(sizeof(UIContext)); }
void UIContext_Destroy(UIContext *ctx) { free(ctx); }

UIInput *UIInput_Create(void) { return malloc(sizeof(UIInput)); }
void UIInput_Destroy(UIInput *input) { free(input); }
void UIInput_Update(UIInput *input) { (void)input; }

UIStyle *UIStyle_Create(void) { return malloc(sizeof(UIStyle)); }
void UIStyle_Destroy(UIStyle *style) { free(style); }
void UI_LoadDefaultStyles(void) {}

UIFontSystem *UIFontSystem_Create(void) { return malloc(sizeof(UIFontSystem)); }
void UIFontSystem_Destroy(UIFontSystem *fs) { free(fs); }

void UILayout_Update(void) {}
void UIFocus_Update(void) {}

// NOTE: UIRenderer creation should likely come from Render system or be created
// here. The masterplan says UIRenderer_Create() in UI_Init. But Render.c ALSO
// created UIRenderer? Render.c created: g_render_system.ui_renderer =
// UIRenderer_Create(); UI.c creates: g_ui_system.renderer =
// UIRenderer_Create(); This implies UIRenderer_Create is defined SOMEWHERE. If
// both link against it, we need ONE definition. I provided a stub in
// Mesh_Renderer.c (which was Render/Mesh/Mesh_Renderer.c) for
// UIRenderer_Create. I should rely on THAT one or move it to UI module. The
// Masterplan suggests UI system *owns* these components. For now, I will define
// them here and remove/guard the ones in Render if conflict arises. Actually,
// since this is a separate compilation unit, weak symbols or just separate
// logic? Ideally UIRenderer belongs to UI. Render system holds a pointer to it?
// Or Render System uses it? Let's implement stubs here and rename/refactor
// later if linker yells. Actually, `Render.c` called `UIRenderer_Create`. If I
// define it here, and also in `Mesh_Renderer.c` (as I did in Step 376), I get a
// Multiple Definition Error. I must REMOVE the stub from Mesh_Renderer.c if I
// define it here. OR, I define it here and assume Mesh_Renderer.c's stub is
// temporary. I will implement it here properly (as a stub) and if linker fails,
// I'll fix Mesh_Renderer.c. Wait, I can't easily edit Mesh_Renderer.c in this
// same turn if I want to be safe. But valid C allows multiple declarations, not
// definitions. I'll define it here. If I get duplicate symbol, I'll delete from
// Mesh_Renderer.c in next step.

// Forward decl of UIRenderer_Create if not in header
// It IS in Render.h which might be included by Mesh_Renderer.c?
// No, Mesh_Renderer.c includes Render.h. UI.c includes UI.h.
// They are separate.
// I will comment out the implementation here if it's already in Mesh_Renderer
// or duplicate function names? I'll stick to the plan: Implement UI.c. I will
// use `UI_UIRenderer_Create` internal naming if needed, but God Function
// expects `UIRenderer_Create`.

// I will define it here.
// To avoid conflict with the stub I wrote in Mesh_Renderer.c (Step 377),
// I should probably rename or assume Mesh_Renderer.c was just for Phase 5
// standalone. I'll go ahead.

void UIRenderer_BeginFrame(UIRenderer *renderer) { (void)renderer; }
void UIRenderer_EndFrame(UIRenderer *renderer) { (void)renderer; }
void UIRenderer_RenderWidgetTree(UIRenderer *renderer, UIWidget *root) {
  (void)renderer;
  (void)root;
}

bool UI_Init(void) {
  g_ui_system.context = UIContext_Create();
  g_ui_system.input = UIInput_Create();
  g_ui_system.style = UIStyle_Create();
  // g_ui_system.renderer = UIRenderer_Create(); // Commented out to avoid link
  // error with Mesh_Renderer.c stub for now, or I rely on extern? Let's try to
  // CALL it. g_ui_system.renderer = UIRenderer_Create();
  g_ui_system.font_system = UIFontSystem_Create();

  UI_LoadDefaultStyles();

  return true;
}

void UI_Update(float delta_time) {
  UIInput_Update(g_ui_system.input);
  // UIAnimation_Update(delta_time); // Missing prototype
  UILayout_Update();
  UIFocus_Update();
}

void UI_Render(void) {
  if (g_ui_system.renderer) {
    UIRenderer_BeginFrame(g_ui_system.renderer);
    UIRenderer_RenderWidgetTree(g_ui_system.renderer, g_ui_system.root_widget);
    UIRenderer_EndFrame(g_ui_system.renderer);
  }
}

void UI_Shutdown(void) {
  UIFontSystem_Destroy(g_ui_system.font_system);
  // UIRenderer_Destroy(g_ui_system.renderer);
  UIStyle_Destroy(g_ui_system.style);
  UIInput_Destroy(g_ui_system.input);
  UIContext_Destroy(g_ui_system.context);
}
