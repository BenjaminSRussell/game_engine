#ifndef ULTIMATE_ENGINE_UI_H
#define ULTIMATE_ENGINE_UI_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Structures
typedef struct UIContext UIContext;
typedef struct UIInput UIInput;
typedef struct UIStyle UIStyle;
typedef struct UIRenderer UIRenderer;
typedef struct UIFontSystem UIFontSystem;
typedef struct UIWidget UIWidget;

typedef struct UISystem {
  UIContext *context;
  UIInput *input;
  UIStyle *style;
  UIRenderer *renderer;
  UIFontSystem *font_system;
  UIWidget *root_widget;
} UISystem;

// Lifecycle
bool UI_Init(void);
void UI_Shutdown(void);
void UI_Update(float delta_time);
void UI_Render(void);

// Subsystems
UIContext *UIContext_Create(void);
void UIContext_Destroy(UIContext *ctx);

UIInput *UIInput_Create(void);
void UIInput_Destroy(UIInput *input);
void UIInput_Update(UIInput *input);

UIStyle *UIStyle_Create(void);
void UIStyle_Destroy(UIStyle *style);
void UI_LoadDefaultStyles(void);

// UIRenderer is likely the same type as in Render.h, but forward declared here.
// To avoid conflict, we might need a bridge or just assume opaque pointer
// compatibility. For now, defining local prototypes.

// UIRenderer functions
void UIRenderer_BeginFrame(UIRenderer *renderer);
void UIRenderer_EndFrame(UIRenderer *renderer);
void UIRenderer_RenderWidgetTree(UIRenderer *renderer, UIWidget *root);

UIFontSystem *UIFontSystem_Create(void);
void UIFontSystem_Destroy(UIFontSystem *fs);

void UILayout_Update(void);
void UIFocus_Update(void);

#endif // ULTIMATE_ENGINE_UI_H
