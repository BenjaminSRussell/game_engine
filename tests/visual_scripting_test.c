/*
 * visual_scripting_test.c
 * Node-graph evaluation in visual_scripting.c. A flow link must reach the node
 * that owns its input pin, vs_graph_execute must walk the links in order, and
 * the walk must stop where the flow ends. Compiles visual_scripting.c directly
 * and does not link Engine.
 */

#include "core/memory/unified_memory_allocator.h"
#include "scripting/visual_scripting.h"

#include <stdio.h>
#include <stdlib.h>

void *unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags,
                           const char *file, int line, const char *function) {
    (void)strategy;
    (void)flags;
    (void)file;
    (void)line;
    (void)function;
    return calloc(1, size > 0 ? size : 1);
}

void unified_memory_free(void *ptr, const char *file, int line, const char *function) {
    (void)file;
    (void)line;
    (void)function;
    free(ptr);
}

static void check(int ok, const char *msg) {
    if (!ok) {
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(1);
    }
}

static VSPinID flow_in(const VSGraph *graph, VSNodeID node) {
    return graph->nodes[node].inputs[0].id;
}

static VSPinID flow_out(const VSGraph *graph, VSNodeID node) {
    return graph->nodes[node].outputs[0].id;
}

/* Entry -> First -> Second: the walk must follow both links and stop at Second. */
static void test_linear_chain_reaches_last_node(void) {
    VSGraph *graph = vs_graph_create();
    check(graph != NULL, "create graph");

    VSNodeID entry = vs_graph_add_node(graph, "Entry", VS_NODE_ENTRY);
    VSNodeID first = vs_graph_add_node(graph, "First", VS_NODE_PRINT);
    VSNodeID second = vs_graph_add_node(graph, "Second", VS_NODE_PRINT);
    check(entry != 0xFFFFFFFFu && first != 0xFFFFFFFFu && second != 0xFFFFFFFFu,
          "add nodes");

    check(graph->nodes[first].inputs[0].node_id == first,
          "input pin records its owning node");
    check(graph->nodes[first].outputs[0].node_id == first,
          "output pin records its owning node");

    vs_graph_connect(graph, flow_out(graph, entry), flow_in(graph, first));
    vs_graph_connect(graph, flow_out(graph, first), flow_in(graph, second));
    check(graph->nodes[entry].outputs[0].is_connected, "entry output connected");
    check(graph->nodes[second].inputs[0].is_connected, "second input connected");

    vs_graph_execute(graph, entry);
    check(graph->current_node == second, "walk ends on the last node of the chain");

    vs_graph_destroy(graph);
    printf("PASS: linear chain reaches last node\n");
}

/* A link to a pin that does not exist must end the flow, not crash or loop. */
static void test_link_to_missing_pin_ends_flow(void) {
    VSGraph *graph = vs_graph_create();
    check(graph != NULL, "create graph");

    VSNodeID entry = vs_graph_add_node(graph, "Entry", VS_NODE_ENTRY);
    check(entry != 0xFFFFFFFFu, "add entry");
    vs_graph_connect(graph, flow_out(graph, entry), 0xDEADBEEFu);

    vs_graph_execute(graph, entry);
    check(graph->current_node == entry, "dangling link ends the flow at entry");

    vs_graph_destroy(graph);
    printf("PASS: link to missing pin ends flow\n");
}

int main(void) {
    test_linear_chain_reaches_last_node();
    test_link_to_missing_pin_ends_flow();
    printf("PASS: visual script graph evaluation\n");
    return 0;
}
