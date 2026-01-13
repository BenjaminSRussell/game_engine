/**
 * =================================================================================================
 *                           DOCUMENTATION GENERATOR SYSTEM
 *                                  Agent: AGENT_DOCS_1
 * =================================================================================================
 *
 * Automatic documentation generation from code comments.
 * Generates HTML, Markdown, and searchable API references.
 *
 * =================================================================================================
 */

#ifndef DOCUMENTATION_GENERATOR_H
#define DOCUMENTATION_GENERATOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* =================================================================================================
 *                                    DOCUMENTATION ELEMENTS
 * =================================================================================================
 */

typedef enum DocElementType {
  DOC_FUNCTION,
  DOC_STRUCT,
  DOC_ENUM,
  DOC_DEFINE,
  DOC_TYPEDEF,
  DOC_VARIABLE,
  DOC_FILE,
  DOC_MODULE,
} DocElementType;

typedef struct DocParameter {
  char name[64];
  char type[64];
  char description[256];
  bool is_optional;
  char default_value[64];
} DocParameter;

typedef struct DocElement {
  DocElementType type;
  char name[128];
  char brief[256];
  char detailed[2048];
  char file_path[256];
  uint32_t line_number;
  char return_type[64];
  char return_desc[256];
  DocParameter params[16];
  uint32_t param_count;
  char see_also[8][64];
  uint32_t see_also_count;
  char examples[4096];
  bool is_deprecated;
  char deprecated_msg[256];
  char since_version[16];
} DocElement;

// Doxygen comment parser implementation
typedef struct DoxygenComment {
  char brief[256];
  char detailed[2048];
  char params[16][128];
  uint32_t param_count;
  char return_desc[512];
  char see_also[8][64];
  uint32_t see_also_count;
  char since[32];
  char deprecated[256];
} DoxygenComment;

DoxygenComment* doxygen_parse_comment(const char* comment_text) {
  if (!comment_text) return NULL;
  
  DoxygenComment* doc = malloc(sizeof(DoxygenComment));
  if (!doc) return NULL;
  
  memset(doc, 0, sizeof(DoxygenComment));
  
  // Parse comment line by line
  char* lines[64];
  uint32_t line_count = 0;
  char* comment_copy = strdup(comment_text);
  char* line = strtok(comment_copy, "\n");
  
  while (line && line_count < 64) {
    // Remove leading whitespace and comment markers
    while (*line == ' ' || *line == '\t' || *line == '*' || *line == '/') line++;
    
    lines[line_count++] = strdup(line);
    line = strtok(NULL, "\n");
  }
  
  // Parse Doxygen commands
  for (uint32_t i = 0; i < line_count; i++) {
    if (strncmp(lines[i], "@brief ", 7) == 0) {
      strncpy(doc->brief, lines[i] + 7, 255);
      doc->brief[255] = '\0';
    } else if (strncmp(lines[i], "@param ", 7) == 0) {
      if (doc->param_count < 16) {
        strncpy(doc->params[doc->param_count], lines[i] + 7, 127);
        doc->params[doc->param_count][127] = '\0';
        doc->param_count++;
      }
    } else if (strncmp(lines[i], "@return ", 8) == 0) {
      strncpy(doc->return_desc, lines[i] + 8, 511);
      doc->return_desc[511] = '\0';
    } else if (strncmp(lines[i], "@see ", 5) == 0) {
      if (doc->see_also_count < 8) {
        strncpy(doc->see_also[doc->see_also_count], lines[i] + 5, 63);
        doc->see_also[doc->see_also_count][63] = '\0';
        doc->see_also_count++;
      }
    } else if (strncmp(lines[i], "@since ", 7) == 0) {
      strncpy(doc->since, lines[i] + 7, 31);
      doc->since[31] = '\0';
    } else if (strncmp(lines[i], "@deprecated ", 11) == 0) {
      strncpy(doc->deprecated, lines[i] + 11, 255);
      doc->deprecated[255] = '\0';
    } else if (lines[i][0] != '@') {
      // Accumulate detailed description
      if (strlen(doc->detailed) + strlen(lines[i]) < 2047) {
        strcat(doc->detailed, lines[i]);
        strcat(doc->detailed, " ");
      }
    }
  }
  
  // Cleanup
  free(comment_copy);
  for (uint32_t i = 0; i < line_count; i++) {
    free(lines[i]);
  }
  
  return doc;
}

// Javadoc comment parser implementation
typedef struct JavadocComment {
  char summary[256];
  char description[2048];
  char params[16][128];
  uint32_t param_count;
  char return_desc[512];
  char throws[8][128];
  uint32_t throws_count;
  char since[32];
  char deprecated[256];
  char author[128];
} JavadocComment;

JavadocComment* javadoc_parse_comment(const char* comment_text) {
  if (!comment_text) return NULL;
  
  JavadocComment* doc = malloc(sizeof(JavadocComment));
  if (!doc) return NULL;
  
  memset(doc, 0, sizeof(JavadocComment));
  
  // Parse comment line by line
  char* lines[64];
  uint32_t line_count = 0;
  char* comment_copy = strdup(comment_text);
  char* line = strtok(comment_copy, "\n");
  
  while (line && line_count < 64) {
    // Remove leading whitespace and comment markers
    while (*line == ' ' || *line == '\t' || *line == '*' || *line == '/') line++;
    
    lines[line_count++] = strdup(line);
    line = strtok(NULL, "\n");
  }
  
  // Parse Javadoc tags
  for (uint32_t i = 0; i < line_count; i++) {
    if (strncmp(lines[i], "@summary ", 9) == 0) {
      strncpy(doc->summary, lines[i] + 9, 255);
      doc->summary[255] = '\0';
    } else if (strncmp(lines[i], "@param ", 7) == 0) {
      if (doc->param_count < 16) {
        strncpy(doc->params[doc->param_count], lines[i] + 7, 127);
        doc->params[doc->param_count][127] = '\0';
        doc->param_count++;
      }
    } else if (strncmp(lines[i], "@return ", 8) == 0) {
      strncpy(doc->return_desc, lines[i] + 8, 511);
      doc->return_desc[511] = '\0';
    } else if (strncmp(lines[i], "@throws ", 8) == 0) {
      if (doc->throws_count < 8) {
        strncpy(doc->throws[doc->throws_count], lines[i] + 8, 127);
        doc->throws[doc->throws_count][127] = '\0';
        doc->throws_count++;
      }
    } else if (strncmp(lines[i], "@since ", 7) == 0) {
      strncpy(doc->since, lines[i] + 7, 31);
      doc->since[31] = '\0';
    } else if (strncmp(lines[i], "@deprecated ", 11) == 0) {
      strncpy(doc->deprecated, lines[i] + 11, 255);
      doc->deprecated[255] = '\0';
    } else if (strncmp(lines[i], "@author ", 8) == 0) {
      strncpy(doc->author, lines[i] + 8, 127);
      doc->author[127] = '\0';
    } else if (lines[i][0] != '@') {
      // Accumulate description
      if (strlen(doc->description) + strlen(lines[i]) < 2047) {
        strcat(doc->description, lines[i]);
        strcat(doc->description, " ");
      }
    }
  }
  
  // Cleanup
  free(comment_copy);
  for (uint32_t i = 0; i < line_count; i++) {
    free(lines[i]);
  }
  
  return doc;
}

// C header parser implementation
typedef struct CHeaderParser {
  char* file_content;
  char* current_position;
  char file_path[256];
  DocElement* elements;
  uint32_t element_count;
  uint32_t capacity;
} CHeaderParser;

CHeaderParser* cheader_parser_init(const char* file_path) {
  if (!file_path) return NULL;
  
  CHeaderParser* parser = malloc(sizeof(CHeaderParser));
  if (!parser) return NULL;
  
  memset(parser, 0, sizeof(CHeaderParser));
  strncpy(parser->file_path, file_path, 255);
  parser->file_path[255] = '\0';
  
  // Read file content
  FILE* file = fopen(file_path, "r");
  if (!file) {
    free(parser);
    return NULL;
  }
  
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  parser->file_content = malloc(file_size + 1);
  if (!parser->file_content) {
    fclose(file);
    free(parser);
    return NULL;
  }
  
  fread(parser->file_content, 1, file_size, file);
  parser->file_content[file_size] = '\0';
  fclose(file);
  
  parser->current_position = parser->file_content;
  parser->capacity = 64;
  parser->elements = malloc(sizeof(DocElement) * parser->capacity);
  
  return parser;
}

void cheader_parser_shutdown(CHeaderParser* parser) {
  if (!parser) return;
  
  if (parser->file_content) {
    free(parser->file_content);
  }
  
  if (parser->elements) {
    for (uint32_t i = 0; i < parser->element_count; i++) {
      // Cleanup element fields if needed
    }
    free(parser->elements);
  }
  
  free(parser);
}

// Struct member extraction implementation
typedef struct StructMember {
  char type[64];
  char name[64];
  char description[256];
  bool is_pointer;
  bool is_array;
  uint32_t array_size;
  char bitfield_size[16];
} StructMember;

StructMember* struct_extract_members(const char* struct_definition, uint32_t* member_count) {
  if (!struct_definition || !member_count) return NULL;
  
  // Find struct body
  const char* body_start = strchr(struct_definition, '{');
  if (!body_start) return NULL;
  
  const char* body_end = strchr(body_start, '}');
  if (!body_end) return NULL;
  
  // Count members first
  uint32_t count = 0;
  const char* ptr = body_start + 1;
  while (ptr < body_end) {
    if (*ptr == ';') count++;
    ptr++;
  }
  
  if (count == 0) return NULL;
  
  StructMember* members = malloc(sizeof(StructMember) * count);
  if (!members) return NULL;
  
  memset(members, 0, sizeof(StructMember) * count);
  
  // Parse members
  uint32_t index = 0;
  ptr = body_start + 1;
  char line[256];
  
  while (ptr < body_end && index < count) {
    // Skip whitespace
    while (ptr < body_end && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')) ptr++;
    
    // Extract member line
    const char* line_start = ptr;
    while (ptr < body_end && *ptr != ';') ptr++;
    
    uint32_t line_len = ptr - line_start;
    if (line_len < 255) {
      strncpy(line, line_start, line_len);
      line[line_len] = '\0';
      
      // Parse type and name
      char* last_space = strrchr(line, ' ');
      if (last_space) {
        *last_space = '\0';
        
        // Extract type
        strncpy(members[index].type, line, 63);
        members[index].type[63] = '\0';
        
        // Extract name (remove any array brackets)
        char* name_start = last_space + 1;
        char* array_start = strchr(name_start, '[');
        
        if (array_start) {
          *array_start = '\0';
          members[index].is_array = true;
          members[index].array_size = atoi(array_start + 1);
        }
        
        strncpy(members[index].name, name_start, 63);
        members[index].name[63] = '\0';
        
        // Check for pointer
        if (strchr(members[index].type, '*')) {
          members[index].is_pointer = true;
        }
        
        index++;
      }
    }
    
    ptr++; // Skip semicolon
  }
  
  *member_count = index;
  return members;
}
int parse_enum_values(const char *content, DocElement *element);
int parse_function_signature(const char *content, DocElement *element);
int resolve_cross_references(DocElement *elements, uint32_t count);
int process_see_links(const char *comment, DocElement *element);
int extract_examples(const char *comment, DocElement *element);

/* =================================================================================================
 *                                    OUTPUT GENERATORS
 * =================================================================================================
 */

typedef struct HTMLGeneratorConfig {
  char output_dir[256];
  char template_dir[256];
  char css_path[256];
  bool generate_search_index;
  bool syntax_highlight_examples;
  char theme_name[32];
} HTMLGeneratorConfig;

int generate_html_index(const DocElement *elements, uint32_t count, const HTMLGeneratorConfig *config);
int generate_html_function(const DocElement *element, const HTMLGeneratorConfig *config);
int generate_html_struct(const DocElement *element, const HTMLGeneratorConfig *config);
int generate_html_navigation(const DocElement *elements, uint32_t count, const HTMLGeneratorConfig *config);
int generate_html_breadcrumbs(const DocElement *element, const HTMLGeneratorConfig *config);
int generate_html_syntax_highlighting(const char *code, char *output, size_t output_size);
int generate_html_search_page(const SearchIndex *index, const HTMLGeneratorConfig *config);
int generate_markdown_output(const DocElement *elements, uint32_t count, const char *output_dir);
int generate_man_page_output(const DocElement *elements, uint32_t count, const char *output_dir);

/* =================================================================================================
 *                                    SEARCH INDEX
 * =================================================================================================
 */

typedef struct SearchIndex {
  struct {
    char term[64];
    uint32_t doc_ids[32];
    uint32_t count;
  } *terms;
  uint32_t term_count;
  DocElement *documents;
  uint32_t document_count;
} SearchIndex;

int build_search_index(const DocElement *elements, uint32_t count, SearchIndex *index);
int tokenize_text(const char *text, char (*tokens)[64], uint32_t *token_count);
int fuzzy_match(const char *query, const char *term, float *score);
int rank_search_results(const char *query, const uint32_t *doc_ids, uint32_t count, float *scores);
int highlight_search_terms(const char *text, const char *terms[], uint32_t term_count, char *output, size_t output_size);
int serialize_search_index(const SearchIndex *index, const char *filepath);

 *                                    DOCUMENTATION API
 * =================================================================================================
 */

typedef struct DocGenerator DocGenerator;
DocGenerator* doc_generator_init(const char *source_dir, const char *output_dir);
int doc_generator_parse_file(DocGenerator *gen, const char *filepath);
int doc_generator_parse_directory(DocGenerator *gen, const char *dirpath);
int doc_generator_generate_html(DocGenerator *gen);
int doc_generator_generate_markdown(DocGenerator *gen, const char *output_dir);
int doc_generator_build_search_index(DocGenerator *gen);
bool doc_generator_serve_local(int port);
bool doc_generator_watch_changes(const char *directory);

#endif // DOCUMENTATION_GENERATOR_H
