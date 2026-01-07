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

// TODO(AGENT_DOCS_1): Implement Doxygen comment parser [Difficulty: 7]
// TODO(AGENT_DOCS_1): Implement Javadoc comment parser [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement C header parser [Difficulty: 7]
// TODO(AGENT_DOCS_1): Implement struct member extraction [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement enum value extraction [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement function signature parsing [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement cross-reference resolution [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement @see/@link processing [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement @example code extraction [Difficulty: 5]

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

// TODO(AGENT_DOCS_1): Implement HTML index page generation [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement HTML function page generation [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement HTML struct page generation [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement HTML navigation tree [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement HTML breadcrumb navigation [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement HTML syntax highlighting [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement HTML search page [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement Markdown output [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement man page output [Difficulty: 5]

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

// TODO(AGENT_DOCS_1): Implement search index building [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement term tokenization [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement fuzzy matching [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement search ranking [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement search result highlighting [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement search index serialization [Difficulty: 5]

/* =================================================================================================
 *                                    DOCUMENTATION API
 * =================================================================================================
 */

// TODO(AGENT_DOCS_1): Implement doc_generator_init [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement doc_generator_parse_file [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement doc_generator_parse_directory [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement doc_generator_generate_html [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement doc_generator_generate_markdown [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement doc_generator_build_search_index [Difficulty:
// 5]
// TODO(AGENT_DOCS_1): Implement doc_generator_serve_local [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement doc_generator_watch_changes [Difficulty: 5]

#endif // DOCUMENTATION_GENERATOR_H
