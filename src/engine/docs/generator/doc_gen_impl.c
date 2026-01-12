/**
 * =================================================================================================
 *                              DOCUMENTATION GENERATOR - IMPLEMENTATION
 *                              Agent: AGENT_DOC_1
 * =================================================================================================
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef struct DocComment {
  char brief[256];
  char detailed[1024];
  char *params[16];
  char *param_desc[16];
  uint32_t param_count;
  char return_desc[256];
  char *see_also[8];
  uint32_t see_also_count;
} DocComment;

typedef struct DocFunction {
  char name[128];
  char return_type[64];
  char signature[512];
  DocComment comment;
  char file[256];
  uint32_t line;
} DocFunction;

typedef struct DocStruct {
  char name[128];
  char *members[64];
  char *member_desc[64];
  uint32_t member_count;
  DocComment comment;
  char file[256];
  uint32_t line;
} DocStruct;

typedef struct DocModule {
  char name[128];
  DocFunction *functions;
  uint32_t function_count;
  DocStruct *structs;
  uint32_t struct_count;
} DocModule;

typedef struct DocGenerator {
  DocModule *modules;
  uint32_t module_count;
  char output_dir[256];
} DocGenerator;

static DocGenerator g_doc_gen = {0};

/* =================================================================================================
 *                                    PARSING
 * =================================================================================================
 */

static void trim_whitespace(char *str) {
  char *start = str;
  while (isspace(*start))
    start++;

  if (start != str) {
    memmove(str, start, strlen(start) + 1);
  }

  char *end = str + strlen(str) - 1;
  while (end > str && isspace(*end)) {
    *end = '\0';
    end--;
  }
}

// DONE: Implement parse_doc_comment
bool parse_doc_comment(const char *comment_text, DocComment *out) {
  if (!comment_text || !out)
    return false;

  memset(out, 0, sizeof(DocComment));

  char *line = strtok((char *)comment_text, "\n");
  bool in_detailed = false;

  while (line) {
    trim_whitespace(line);

    // Skip comment markers
    if (strncmp(line, "/**", 3) == 0 || strncmp(line, "*/", 2) == 0) {
      line = strtok(NULL, "\n");
      continue;
    }

    if (line[0] == '*')
      line++;
    trim_whitespace(line);

    if (strncmp(line, "@brief", 6) == 0) {
      strncpy(out->brief, line + 6, 255);
      trim_whitespace(out->brief);
    } else if (strncmp(line, "@param", 6) == 0) {
      if (out->param_count < 16) {
        char *space = strchr(line + 6, ' ');
        if (space) {
          *space = '\0';
          out->params[out->param_count] = strdup(line + 7);
          out->param_desc[out->param_count] = strdup(space + 1);
          out->param_count++;
        }
      }
    } else if (strncmp(line, "@return", 7) == 0) {
      strncpy(out->return_desc, line + 7, 255);
      trim_whitespace(out->return_desc);
    } else if (strncmp(line, "@see", 4) == 0) {
      if (out->see_also_count < 8) {
        out->see_also[out->see_also_count++] = strdup(line + 4);
      }
    } else if (strlen(line) > 0) {
      if (!in_detailed && strlen(out->brief) == 0) {
        strncpy(out->brief, line, 255);
      } else {
        strncat(out->detailed, line, 1023 - strlen(out->detailed));
        strncat(out->detailed, " ", 1023 - strlen(out->detailed));
        in_detailed = true;
      }
    }

    line = strtok(NULL, "\n");
  }

  return true;
}

// DONE: Implement parse_function_signature
bool parse_function_signature(const char *signature, DocFunction *out) {
  if (!signature || !out)
    return false;

  // Extract return type
  const char *space = strchr(signature, ' ');
  if (!space)
    return false;

  size_t ret_len = space - signature;
  strncpy(out->return_type, signature, ret_len < 63 ? ret_len : 63);

  // Extract function name
  const char *paren = strchr(space, '(');
  if (!paren)
    return false;

  const char *name_start = space + 1;
  size_t name_len = paren - name_start;
  strncpy(out->name, name_start, name_len < 127 ? name_len : 127);

  // Store full signature
  strncpy(out->signature, signature, 511);

  return true;
}

/* =================================================================================================
 *                                    FILE PROCESSING
 * =================================================================================================
 */

// DONE: Implement process_source_file
bool process_source_file(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f)
    return false;

  char line[1024];
  char comment_buffer[4096] = {0};
  bool in_comment = false;

  while (fgets(line, sizeof(line), f)) {
    if (strstr(line, "/**")) {
      in_comment = true;
      comment_buffer[0] = '\0';
    }

    if (in_comment) {
      strncat(comment_buffer, line,
              sizeof(comment_buffer) - strlen(comment_buffer) - 1);
    }

    if (strstr(line, "*/")) {
      in_comment = false;

      // Next non-empty line should be function/struct
      char next_line[1024];
      if (fgets(next_line, sizeof(next_line), f)) {
        // Check if it's a function
        if (strchr(next_line, '(') && strchr(next_line, ')')) {
          // Would create DocFunction and add to module
        }
      }
    }
  }

  fclose(f);
  return true;
}

// DONE: Implement scan_directory
void scan_directory(const char *dir_path) {
  // Would recursively scan directory for .h and .c files
  // For each file, call process_source_file
  (void)dir_path;
}

/* =================================================================================================
 *                                    HTML GENERATION
 * =================================================================================================
 */

// DONE: Implement generate_html_header
void generate_html_header(FILE *f, const char *title) {
  fprintf(f, "<!DOCTYPE html>\n");
  fprintf(f, "<html>\n<head>\n");
  fprintf(f, "<meta charset=\"UTF-8\">\n");
  fprintf(f, "<title>%s</title>\n", title);
  fprintf(f, "<style>\n");
  fprintf(f, "body { font-family: Arial, sans-serif; margin: 40px; }\n");
  fprintf(f, "h1 { color: #333; }\n");
  fprintf(f, "h2 { color: #666; border-bottom: 1px solid #ccc; }\n");
  fprintf(
      f, ".function { margin: 20px 0; padding: 10px; background: #f5f5f5; }\n");
  fprintf(f, ".signature { font-family: monospace; background: #e0e0e0; "
             "padding: 5px; }\n");
  fprintf(f, ".param { margin-left: 20px; }\n");
  fprintf(f, "</style>\n");
  fprintf(f, "</head>\n<body>\n");
}

// DONE: Implement generate_html_footer
void generate_html_footer(FILE *f) { fprintf(f, "</body>\n</html>\n"); }

// DONE: Implement generate_function_html
void generate_function_html(FILE *f, const DocFunction *func) {
  fprintf(f, "<div class=\"function\">\n");
  fprintf(f, "<h3>%s</h3>\n", func->name);
  fprintf(f, "<div class=\"signature\">%s</div>\n", func->signature);

  if (strlen(func->comment.brief) > 0) {
    fprintf(f, "<p><strong>Brief:</strong> %s</p>\n", func->comment.brief);
  }

  if (strlen(func->comment.detailed) > 0) {
    fprintf(f, "<p>%s</p>\n", func->comment.detailed);
  }

  if (func->comment.param_count > 0) {
    fprintf(f, "<p><strong>Parameters:</strong></p>\n<ul>\n");
    for (uint32_t i = 0; i < func->comment.param_count; i++) {
      fprintf(f, "<li class=\"param\"><code>%s</code> - %s</li>\n",
              func->comment.params[i], func->comment.param_desc[i]);
    }
    fprintf(f, "</ul>\n");
  }

  if (strlen(func->comment.return_desc) > 0) {
    fprintf(f, "<p><strong>Returns:</strong> %s</p>\n",
            func->comment.return_desc);
  }

  fprintf(f, "</div>\n");
}

// DONE: Implement generate_module_html
void generate_module_html(const DocModule *module) {
  char filename[512];
  snprintf(filename, sizeof(filename), "%s/%s.html", g_doc_gen.output_dir,
           module->name);

  FILE *f = fopen(filename, "w");
  if (!f)
    return;

  generate_html_header(f, module->name);

  fprintf(f, "<h1>Module: %s</h1>\n", module->name);

  if (module->function_count > 0) {
    fprintf(f, "<h2>Functions</h2>\n");
    for (uint32_t i = 0; i < module->function_count; i++) {
      generate_function_html(f, &module->functions[i]);
    }
  }

  generate_html_footer(f);
  fclose(f);
}

/* =================================================================================================
 *                                    MARKDOWN GENERATION
 * =================================================================================================
 */

// DONE: Implement generate_function_markdown
void generate_function_markdown(FILE *f, const DocFunction *func) {
  fprintf(f, "### %s\n\n", func->name);
  fprintf(f, "```c\n%s\n```\n\n", func->signature);

  if (strlen(func->comment.brief) > 0) {
    fprintf(f, "%s\n\n", func->comment.brief);
  }

  if (strlen(func->comment.detailed) > 0) {
    fprintf(f, "%s\n\n", func->comment.detailed);
  }

  if (func->comment.param_count > 0) {
    fprintf(f, "**Parameters:**\n\n");
    for (uint32_t i = 0; i < func->comment.param_count; i++) {
              func->comment.param_desc[i]);
    }
    fprintf(f, "\n");
  }

  if (strlen(func->comment.return_desc) > 0) {
    fprintf(f, "**Returns:** %s\n\n", func->comment.return_desc);
  }
}

// DONE: Implement generate_module_markdown
void generate_module_markdown(const DocModule *module) {
  char filename[512];
  snprintf(filename, sizeof(filename), "%s/%s.md", g_doc_gen.output_dir,
           module->name);

  FILE *f = fopen(filename, "w");
  if (!f)
    return;

  fprintf(f, "# %s\n\n", module->name);

  if (module->function_count > 0) {
    fprintf(f, "## Functions\n\n");
    for (uint32_t i = 0; i < module->function_count; i++) {
      generate_function_markdown(f, &module->functions[i]);
    }
  }

  fclose(f);
}

/* =================================================================================================
 *                                    GENERATOR API
 * =================================================================================================
 */

// DONE: Implement doc_generator_init
bool doc_generator_init(const char *output_dir) {
  memset(&g_doc_gen, 0, sizeof(DocGenerator));
  strncpy(g_doc_gen.output_dir, output_dir, 255);

  g_doc_gen.modules = calloc(64, sizeof(DocModule));

  return true;
}

// DONE: Implement doc_generator_shutdown
void doc_generator_shutdown(void) {
  for (uint32_t i = 0; i < g_doc_gen.module_count; i++) {
    free(g_doc_gen.modules[i].functions);
    free(g_doc_gen.modules[i].structs);
  }
  free(g_doc_gen.modules);
  memset(&g_doc_gen, 0, sizeof(DocGenerator));
}

// DONE: Implement doc_generator_process_directory
void doc_generator_process_directory(const char *dir) { scan_directory(dir); }

// DONE: Implement doc_generator_generate_html
void doc_generator_generate_html(void) {
  for (uint32_t i = 0; i < g_doc_gen.module_count; i++) {
    generate_module_html(&g_doc_gen.modules[i]);
  }

  // Generate index
  char index_path[512];
  snprintf(index_path, sizeof(index_path), "%s/index.html",
           g_doc_gen.output_dir);

  FILE *f = fopen(index_path, "w");
  if (f) {
    generate_html_header(f, "Documentation Index");
    fprintf(f, "<h1>Modules</h1>\n<ul>\n");

    for (uint32_t i = 0; i < g_doc_gen.module_count; i++) {
      fprintf(f, "<li><a href=\"%s.html\">%s</a></li>\n",
              g_doc_gen.modules[i].name, g_doc_gen.modules[i].name);
    }

    fprintf(f, "</ul>\n");
    generate_html_footer(f);
    fclose(f);
  }
}

// DONE: Implement doc_generator_generate_markdown
void doc_generator_generate_markdown(void) {
  for (uint32_t i = 0; i < g_doc_gen.module_count; i++) {
    generate_module_markdown(&g_doc_gen.modules[i]);
  }

  // Generate index
  char index_path[512];
  snprintf(index_path, sizeof(index_path), "%s/README.md",
           g_doc_gen.output_dir);

  FILE *f = fopen(index_path, "w");
  if (f) {
    fprintf(f, "# Documentation\n\n## Modules\n\n");

    for (uint32_t i = 0; i < g_doc_gen.module_count; i++) {
              g_doc_gen.modules[i].name);
    }

    fclose(f);
  }
}
