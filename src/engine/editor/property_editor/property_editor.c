#include "property_editor.h"
#include "engine/include/core/logger.h"
#include <editor/editor_main.h>
#include <stdio.h>

#ifdef ENABLE_IMGUI
#include <imgui.h>
#endif

static Entity g_selected_entity = {0}; // Invalid entity
static bool g_has_selection = false;

void PropertyEditor_Init(void) {
    LOG_INFO("Property Editor initialized");
}

void PropertyEditor_Update(void) {
}

void PropertyEditor_Render(void) {
    if (!g_has_selection) {
        return;
    }

#ifdef ENABLE_IMGUI
    if (ImGui::Begin("Inspector")) {
        ImGui::Text("Entity ID: %d", g_selected_entity.id);

        // Placeholder for component inspection
        if (ImGui::CollapsingHeader("Transform")) {
            // ImGui::DragFloat3("Position", &position.x);
            // ImGui::DragFloat3("Rotation", &rotation.x);
            // ImGui::DragFloat3("Scale", &scale.x);
        }
    }
    ImGui::End();
#endif
}

void PropertyEditor_SetSelection(Entity entity) {
    g_selected_entity = entity;
    g_has_selection = true;
    LOG_INFO("Property Editor: Selected entity %d", entity.id);
}

void PropertyEditor_ClearSelection(void) {
    g_has_selection = false;
}
