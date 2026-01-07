/**
 * =================================================================================================
 *                              EDITOR WIDGET LIBRARY
 *                               Agent: AGENT_DOCS_1
 * =================================================================================================
 *
 * Complete UI widget library for editor tools and runtime UI.
 *
 * =================================================================================================
 */

#ifndef EDITOR_WIDGETS_H
#define EDITOR_WIDGETS_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    WIDGET BASE
 * =================================================================================================
 */

typedef struct WidgetStyle {
  float background_color[4];
  float border_color[4];
  float text_color[4];
  float hover_color[4];
  float active_color[4];
  float disabled_color[4];
  float border_width;
  float corner_radius;
  float padding[4];
  float margin[4];
  char font_name[32];
  float font_size;
} WidgetStyle;

typedef struct Widget {
  uint32_t id;
  char name[64];
  uint32_t type_id;
  struct Widget *parent;
  struct Widget **children;
  uint32_t child_count;
  float position[2];
  float size[2];
  float anchor_min[2];
  float anchor_max[2];
  float pivot[2];
  bool is_visible;
  bool is_enabled;
  bool is_interactable;
  bool is_focused;
  WidgetStyle style;
  void *user_data;
} Widget;

Widget *widget_create(const char *name, uint32_t type_id);
void widget_destroy(Widget *widget);
void widget_add_child(Widget *parent, Widget *child);
void widget_remove_child(Widget *parent, Widget *child);
void widget_set_position(Widget *widget, float x, float y);
void widget_set_size(Widget *widget, float w, float h);
void widget_set_anchors(Widget *widget, float min_x, float min_y, float max_x,
                        float max_y);
void widget_calculate_rect(Widget *widget, float *rect);
bool widget_hit_test(Widget *widget, float x, float y);
void widget_render(Widget *widget);
void widget_focus(Widget *widget);
void widget_layout_vertical(Widget *widget, float spacing);
void widget_layout_horizontal(Widget *widget, float spacing);
void widget_layout_grid(Widget *widget, int columns, float spacing);

/* =================================================================================================
 *                                    BUTTON
 * =================================================================================================
 */

typedef struct Button {
  Widget base;
  char text[128];
  uint32_t icon_id;
  bool is_toggle;
  bool is_pressed;
  void (*on_click)(Widget *widget);
  void (*on_press)(Widget *widget);
  void (*on_release)(Widget *widget);
} Button;

Button *button_create(const char *name, const char *text);
void button_set_text(Button *button, const char *text);
void button_set_icon(Button *button, uint32_t icon_id);
void button_render(Button *button);
void button_handle_input(Button *button, void *input_event);

/* =================================================================================================
 *                                    TEXT INPUT
 * =================================================================================================
 */

typedef struct TextInput {
  Widget base;
  char text[1024];
  char placeholder[128];
  uint32_t cursor_position;
  uint32_t selection_start;
  uint32_t selection_end;
  bool is_multiline;
  bool is_password;
  uint32_t max_length;
  char allowed_chars[128];
  void (*on_change)(Widget *widget, const char *text);
  void (*on_submit)(Widget *widget, const char *text);
} TextInput;

TextInput *textinput_create(const char *name, const char *placeholder);
void textinput_set_text(TextInput *input, const char *text);
const char *textinput_get_text(TextInput *input);
void textinput_insert(TextInput *input, const char *text);
void textinput_delete(TextInput *input);
void textinput_select_all(TextInput *input);
void textinput_copy(TextInput *input);
void textinput_paste(TextInput *input);
void textinput_cut(TextInput *input);
void textinput_undo(TextInput *input);
void textinput_redo(TextInput *input);
void textinput_render(TextInput *input);
void textinput_handle_input(TextInput *input, void *input_event);

/* =================================================================================================
 *                                    SLIDER
 * =================================================================================================
 */

typedef struct Slider {
  Widget base;
  float value;
  float min_value;
  float max_value;
  float step;
  bool is_integer;
  bool show_value;
  char format[16];
  void (*on_change)(Widget *widget, float value);
} Slider;

Slider *slider_create(const char *name, float value, float min, float max);
void slider_set_value(Slider *slider, float value);
void slider_set_range(Slider *slider, float min, float max);
void slider_render(Slider *slider);
void slider_handle_input(Slider *slider, void *input_event);
float slider_value_from_position(Slider *slider, float x, float y);

/* =================================================================================================
 *                                    DROPDOWN
 * =================================================================================================
 */

typedef struct Dropdown {
  Widget base;
  char **options;
  uint32_t option_count;
  int32_t selected_index;
  bool is_open;
  float dropdown_height;
  void (*on_selection_change)(Widget *widget, int32_t index);
} Dropdown;

Dropdown *dropdown_create(const char *name);
void dropdown_add_option(Dropdown *dropdown, const char *option);
void dropdown_remove_option(Dropdown *dropdown, int index);
void dropdown_clear_options(Dropdown *dropdown);
void dropdown_set_selected(Dropdown *dropdown, int index);
void dropdown_render(Dropdown *dropdown);
void dropdown_render_popup(Dropdown *dropdown);
void dropdown_handle_input(Dropdown *dropdown, void *input_event);

/* =================================================================================================
 *                                    TREE VIEW
 * =================================================================================================
 */

typedef struct TreeNode {
  uint32_t id;
  char text[128];
  uint32_t icon_id;
  bool is_expanded;
  bool is_selected;
  bool is_leaf;
  struct TreeNode *parent;
  struct TreeNode **children;
  uint32_t child_count;
  void *user_data;
} TreeNode;

typedef struct TreeView {
  Widget base;
  TreeNode *root;
  TreeNode *selected_node;
  bool allow_multi_select;
  TreeNode **selected_nodes;
  uint32_t selected_count;
  float indent_size;
  bool show_root;
  void (*on_selection_change)(Widget *widget, TreeNode *node);
  void (*on_node_expand)(Widget *widget, TreeNode *node);
  void (*on_node_collapse)(Widget *widget, TreeNode *node);
  void (*on_node_double_click)(Widget *widget, TreeNode *node);
} TreeView;

TreeView *treeview_create(const char *name);
TreeNode *treeview_add_node(TreeView *tv, TreeNode *parent, const char *text);
void treeview_remove_node(TreeView *tv, TreeNode *node);
void treeview_clear(TreeView *tv);
void treeview_expand_all(TreeView *tv);
void treeview_collapse_all(TreeView *tv);
void treeview_select_node(TreeView *tv, TreeNode *node);
void treeview_render(TreeView *tv);
void treeview_render_node(TreeView *tv, TreeNode *node, float indent);
void treeview_handle_input(TreeView *tv, void *input_event);
TreeNode *treeview_find_node_at_position(TreeView *tv, float x, float y);

/* =================================================================================================
 *                                    PROPERTY INSPECTOR
 * =================================================================================================
 */

typedef enum PropertyType {
  PROPERTY_INT,
  PROPERTY_FLOAT,
  PROPERTY_BOOL,
  PROPERTY_STRING,
  PROPERTY_VECTOR2,
  PROPERTY_VECTOR3,
  PROPERTY_VECTOR4,
  PROPERTY_COLOR,
  PROPERTY_ENUM,
  PROPERTY_OBJECT_REF,
  PROPERTY_ARRAY,
  PROPERTY_STRUCT,
} PropertyType;

typedef struct PropertyDefinition {
  char name[64];
  char display_name[64];
  PropertyType type;
  void *value_ptr;
  float range_min;
  float range_max;
  char **enum_names;
  uint32_t enum_count;
  char tooltip[256];
  bool is_readonly;
  bool is_hidden;
  void (*on_change)(const char *name, void *value);
} PropertyDefinition;

typedef struct PropertyInspector {
  Widget base;
  PropertyDefinition *properties;
  uint32_t property_count;
  char **categories;
  uint32_t category_count;
  bool *category_expanded;
  float label_width;
  void *target_object;
} PropertyInspector;

PropertyInspector *inspector_create(const char *name);
void inspector_set_target(PropertyInspector *inspector, void *object);
void inspector_add_property(PropertyInspector *inspector,
                            const PropertyDefinition *def);
void inspector_remove_property(PropertyInspector *inspector, const char *name);
void inspector_refresh(PropertyInspector *inspector);
void inspector_render(PropertyInspector *inspector);
void inspector_render_int(PropertyInspector *inspector,
                          PropertyDefinition *prop);
void inspector_render_float(PropertyInspector *inspector,
                            PropertyDefinition *prop);
void inspector_render_bool(PropertyInspector *inspector,
                           PropertyDefinition *prop);
void inspector_render_string(PropertyInspector *inspector,
                             PropertyDefinition *prop);
void inspector_render_vector3(PropertyInspector *inspector,
                              PropertyDefinition *prop);
void inspector_render_color(PropertyInspector *inspector,
                            PropertyDefinition *prop);
void inspector_render_enum(PropertyInspector *inspector,
                           PropertyDefinition *prop);
void inspector_render_object_ref(PropertyInspector *inspector,
                                 PropertyDefinition *prop);
void inspector_render_array(PropertyInspector *inspector,
                            PropertyDefinition *prop);

/* =================================================================================================
 *                                    DOCKING SYSTEM
 * =================================================================================================
 */

typedef enum DockDirection {
  DOCK_LEFT,
  DOCK_RIGHT,
  DOCK_TOP,
  DOCK_BOTTOM,
  DOCK_CENTER,
  DOCK_FLOAT,
} DockDirection;

typedef struct DockPanel {
  uint32_t id;
  char title[64];
  Widget *content;
  DockDirection dock_direction;
  float dock_ratio;
  bool is_closable;
  bool is_maximizable;
  bool is_floating;
  float float_position[2];
  float float_size[2];
} DockPanel;

typedef struct DockSpace {
  Widget base;
  DockPanel *panels;
  uint32_t panel_count;
  struct DockNode *root_node;
  DockPanel *dragging_panel;
  DockDirection drop_target;
} DockSpace;

DockSpace *dockspace_create(const char *name);
void dockspace_add_panel(DockSpace *ds, DockPanel *panel);
void dockspace_remove_panel(DockSpace *ds, DockPanel *panel);
void dockspace_dock_panel(DockSpace *ds, DockPanel *panel, DockDirection dir,
                          DockPanel *target);
void dockspace_undock_panel(DockSpace *ds, DockPanel *panel);
void dockspace_render(DockSpace *ds);
void dockspace_handle_input(DockSpace *ds, void *input_event);
void dockspace_serialize_layout(DockSpace *ds, const char *path);
void dockspace_deserialize_layout(DockSpace *ds, const char *path);
void dockspace_split_node(DockSpace *ds, void *node, DockDirection dir,
                          float ratio);
void dockspace_merge_nodes(DockSpace *ds, void *node_a, void *node_b);
void dockspace_resize_splitter(DockSpace *ds, void *splitter, float delta);

/* =================================================================================================
 *                                    UI SYSTEM
 * =================================================================================================
 */

typedef struct UISystem {
  Widget *root;
  Widget *focused_widget;
  Widget *hovered_widget;
  Widget *dragging_widget;
  WidgetStyle default_style;
  float cursor_position[2];
  bool cursor_visible;
} UISystem;

void ui_system_init(UISystem *sys);
void ui_system_shutdown(UISystem *sys);
void ui_system_update(UISystem *sys, float dt);
void ui_system_render(UISystem *sys);
void ui_system_handle_mouse(UISystem *sys, float x, float y, int button,
                            bool down);
void ui_system_handle_keyboard(UISystem *sys, int key, bool down);
void ui_system_set_focus(UISystem *sys, Widget *widget);
Widget *ui_system_find_widget(UISystem *sys, uint32_t id);
void ui_system_load_theme(UISystem *sys, const char *path);
void ui_system_save_theme(UISystem *sys, const char *path);

#endif // EDITOR_WIDGETS_H
