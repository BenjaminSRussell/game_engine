#include "editor/npc_editor/npc_template_browser.h"
#include <string.h>

typedef struct {
    char name[64];
    char class_name[32];
    char icon_path[64];
    void *template_data;
} KPCTemplate;

static KPCTemplate g_templates[100];
static int g_template_count = 0;

void npc_template_browser_draw_grid(void) {
    // Draw grid of thumbnails
    // ImGui::BeginChild("TemplateGrid");
    // for (int i = 0; i < g_template_count; i++) {
    //    ImGui::Image(g_templates[i].icon_path);
    //    ImGui::Text("%s", g_templates[i].name);
    // }
}

void npc_template_browser_handle_drag(void) {
    // if (ImGui::BeginDragDropSource()) {
    //    ImGui::SetDragDropPayload("NPC_TEMPLATE", &selected_idx, sizeof(int));
    //    ImGui::EndDragDropSource();
    // }
}

void npc_template_browser_create_child(int parent_idx, const char *new_name) {
    // Create new template inheriting from parent
    if (g_template_count >= 100) return;
    KPCTemplate *child = &g_templates[g_template_count++];
    KPCTemplate *parent = &g_templates[parent_idx];
    
    strcpy(child->name, new_name);
    strcpy(child->class_name, parent->class_name);
    // Copy other properties...
}
