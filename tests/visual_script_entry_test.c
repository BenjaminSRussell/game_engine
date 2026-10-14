/*
 * visual_script_entry_test.c
 * vs_graph_execute must reject a null graph or an out-of-range entry node
 * instead of dereferencing it. Compiles visual_scripting.c directly and does
 * not link Engine.
 */

#include "core/memory/unified_memory_allocator.h"
#include "scripting/visual_scripting.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

typedef struct {
    int saved;
    int read_fd;
} Cap;

static void cap_begin(Cap *cap, int fd) {
    int fds[2];
    if (pipe(fds) != 0) {
        fail("pipe");
    }
    if (fd == STDOUT_FILENO) {
        fflush(stdout);
    } else {
        fflush(stderr);
    }
    cap->saved = dup(fd);
    if (cap->saved < 0 || dup2(fds[1], fd) < 0) {
        fail("dup");
    }
    close(fds[1]);
    cap->read_fd = fds[0];
}

static void cap_end(Cap *cap, int fd, char *buf, size_t cap_size) {
    ssize_t n;
    if (fd == STDOUT_FILENO) {
        fflush(stdout);
    } else {
        fflush(stderr);
    }
    if (dup2(cap->saved, fd) < 0) {
        fail("restore");
    }
    close(cap->saved);
    n = read(cap->read_fd, buf, cap_size - 1);
    if (n < 0) {
        n = 0;
    }
    buf[n] = '\0';
    close(cap->read_fd);
}

static void capture_execute(VSGraph *graph, VSNodeID id, char *out, size_t out_n,
                            char *err, size_t err_n) {
    Cap cout_cap;
    Cap cerr_cap;
    cap_begin(&cout_cap, STDOUT_FILENO);
    cap_begin(&cerr_cap, STDERR_FILENO);
    vs_graph_execute(graph, id);
    cap_end(&cout_cap, STDOUT_FILENO, out, out_n);
    cap_end(&cerr_cap, STDERR_FILENO, err, err_n);
}

static void expect_rejected(VSGraph *graph, VSNodeID id, const char *label) {
    char out[1024];
    char err[1024];
    capture_execute(graph, id, out, sizeof out, err, sizeof err);
    if (strstr(out, "Start execution") || strstr(out, "Execution finished")) {
        fprintf(stderr, "FAIL: %s still executed (%s)\n", label, out);
        exit(1);
    }
    if (!strstr(err, "Invalid entry")) {
        fprintf(stderr, "FAIL: %s was not rejected (%s)\n", label, err);
        exit(1);
    }
    printf("PASS: %s\n", label);
}

static void expect_ran(VSGraph *graph, VSNodeID id, const char *label) {
    char out[1024];
    char err[1024];
    capture_execute(graph, id, out, sizeof out, err, sizeof err);
    if (!strstr(out, "Start execution") || !strstr(out, "(Entry)") ||
        !strstr(out, "Execution finished")) {
        fprintf(stderr, "FAIL: %s did not run (%s)\n", label, out);
        exit(1);
    }
    if (strstr(err, "Invalid entry")) {
        fprintf(stderr, "FAIL: %s was rejected (%s)\n", label, err);
        exit(1);
    }
    printf("PASS: %s\n", label);
}

int main(void) {
    VSGraph *graph;
    VSNodeID entry;

    expect_rejected(NULL, 0, "null graph");

    graph = vs_graph_create();
    if (!graph) {
        fail("create");
    }
    expect_rejected(graph, 0, "empty graph");
    expect_rejected(graph, 0xFFFFFFFFu, "empty graph stale id");

    entry = vs_graph_add_node(graph, "Entry", VS_NODE_ENTRY);
    if (entry == 0xFFFFFFFFu) {
        fail("add node");
    }
    expect_ran(graph, entry, "valid entry");
    expect_rejected(graph, entry + 1, "out-of-range entry");
    expect_rejected(graph, 0xFFFFFFFFu, "stale entry");

    vs_graph_destroy(graph);
    printf("PASS: visual script entry rejection\n");
    return 0;
}
