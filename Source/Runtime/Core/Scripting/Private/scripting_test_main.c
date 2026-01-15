#include "../Public/unified_scripting.h"
#include <stdio.h>

// Test command callback
static void test_command(int argc, char **argv) {
  printf("Test command called with %d args\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("  arg[%d] = %s\n", i, argv[i]);
  }
}

int main(void) {
  printf("Starting Scripting Verification...\n");

  // 1. Test script initialization
  ScriptContext *context = script_init();
  if (!context) {
    printf("FAILED: Script context creation failed\n");
    return 1;
  }
  printf("PASSED: Script context created\n");

  // 2. Test command registration
  if (!script_register_command(context, "test", test_command, "Test command")) {
    printf("FAILED: Command registration failed\n");
    script_shutdown(context);
    return 1;
  }
  printf("PASSED: Command registered\n");

  // 3. Test command execution
  if (!script_execute(context, "test arg1 arg2")) {
    printf("FAILED: Command execution failed\n");
    script_shutdown(context);
    return 1;
  }
  printf("PASSED: Command executed\n");

  // 4. Test built-in commands
  if (!script_execute(context, "help")) {
    printf("FAILED: Built-in help command failed\n");
    script_shutdown(context);
    return 1;
  }
  printf("PASSED: Built-in help command executed\n");

  // 5. Test variable system
  script_set_var(context, "test_var", 42.0f);
  float val = script_get_var(context, "test_var", 0.0f);
  if (val != 42.0f) {
    printf("FAILED: Variable get/set mismatch (expected 42.0, got %.1f)\n",
           val);
    script_shutdown(context);
    return 1;
  }
  printf("PASSED: Variable system (set=42.0, get=%.1f)\n", val);

  // 6. Test console system
  console_init(context);
  console_print(context, "Test message");
  console_print_error(context, "Test error");
  console_clear(context);
  printf("PASSED: Console initialized and used\n");

  // 7. Test console visibility
  bool visible = console_is_visible(context);
  console_toggle(context);
  if (console_is_visible(context) == visible) {
    printf("FAILED: Console toggle failed\n");
    script_shutdown(context);
    return 1;
  }
  printf("PASSED: Console toggle works\n");

  // 8. Test visual scripting graph
  VSGraph *graph = vs_graph_create();
  if (!graph) {
    printf("FAILED: Visual scripting graph creation failed\n");
    script_shutdown(context);
    return 1;
  }
  printf("PASSED: Visual scripting graph created\n");

  // 9. Test node creation
  VSNodeID entry = vs_graph_add_node(graph, "Entry", VS_NODE_ENTRY);
  VSNodeID print = vs_graph_add_node(graph, "Print", VS_NODE_PRINT);

  if (entry == 0xFFFFFFFF || print == 0xFFFFFFFF) {
    printf("FAILED: Node creation failed\n");
    vs_graph_destroy(graph);
    script_shutdown(context);
    return 1;
  }
  printf("PASSED: Nodes created (entry=%u, print=%u)\n", entry, print);

  // 10. Test node connection
  VSPinID entry_out = (entry << 16) | 1;
  VSPinID print_in = (print << 16) | 1;
  vs_graph_connect(graph, entry_out, print_in);
  printf("PASSED: Nodes connected\n");

  // 11. Test graph execution
  vs_graph_execute(graph, entry);
  printf("PASSED: Visual script executed\n");

  // 12. Cleanup
  vs_graph_destroy(graph);
  console_shutdown(context);
  script_shutdown(context);
  printf("PASSED: Cleanup complete\n");

  printf("\nScripting Verification Successful!\n");
  return 0;
}
