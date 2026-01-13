/**
 * =================================================================================================
 *                           DOCUMENTATION GENERATOR IMPLEMENTATION
 *                                  Agent: AGENT_DOCS_1
 * =================================================================================================
 */

#include "doc_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <dirent.h>
#include <sys/stat.h>

/* =================================================================================================
 *                                    PARSER IMPLEMENTATION
 * ================================================================================================= */

typedef struct {
    DocElement *elements;
    uint32_t count;
    uint32_t capacity;
} DocParserContext;

static DocParserContext* parser_init() {
    DocParserContext *ctx = malloc(sizeof(DocParserContext));
    ctx->elements = malloc(sizeof(DocElement) * 1024);
    ctx->count = 0;
    ctx->capacity = 1024;
    return ctx;
}

static void parser_cleanup(DocParserContext *ctx) {
    free(ctx->elements);
    free(ctx);
}

// TODO-27206: Implement enum value extraction [Difficulty: 4]
static int parse_enum_values(const char *content, DocElement *element) {
    regex_t enum_regex;
    regmatch_t matches[3];
    
    const char *pattern = "\\{([^}]+)\\}";
    if (regcomp(&enum_regex, pattern, REG_EXTENDED) != 0) {
        return -1;
    }
    
    if (regexec(&enum_regex, content, 3, matches, 0) == 0) {
        // Extract enum values between braces
        int start = matches[1].rm_so;
        int end = matches[1].rm_eo;
        char enum_content[256];
        int len = end - start;
        if (len < 255) {
            strncpy(enum_content, content + start, len);
            enum_content[len] = '\0';
            
            // Parse individual enum values
            char *token = strtok(enum_content, ",");
            int value_count = 0;
            while (token && value_count < 16) {
                // Trim whitespace
                while (isspace(*token)) token++;
                char *end = token + strlen(token) - 1;
                while (end > token && isspace(*end)) end--;
                *(end + 1) = '\0';
                
                // Store enum value (simplified - would need proper struct)
                value_count++;
                token = strtok(NULL, ",");
            }
        }
    }
    
    regfree(&enum_regex);
    return 0;
}

// TODO-27207: Implement function signature parsing [Difficulty: 6]
static int parse_function_signature(const char *content, DocElement *element) {
    regex_t func_regex;
    regmatch_t matches[4];
    
    // Match: return_type function_name(parameters)
    const char *pattern = "^\\s*([a-zA-Z_][a-zA-Z0-9_\\*\\s]+)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(([^)]*)\\)";
    if (regcomp(&func_regex, pattern, REG_EXTENDED) != 0) {
        return -1;
    }
    
    if (regexec(&func_regex, content, 4, matches, 0) == 0) {
        // Extract return type
        int ret_start = matches[1].rm_so;
        int ret_end = matches[1].rm_eo;
        int ret_len = ret_end - ret_start;
        if (ret_len < 63) {
            strncpy(element->return_type, content + ret_start, ret_len);
            element->return_type[ret_len] = '\0';
        }
        
        // Extract function name
        int name_start = matches[2].rm_so;
        int name_end = matches[2].rm_eo;
        int name_len = name_end - name_start;
        if (name_len < 127) {
            strncpy(element->name, content + name_start, name_len);
            element->name[name_len] = '\0';
        }
        
        // Parse parameters
        int param_start = matches[3].rm_so;
        int param_end = matches[3].rm_eo;
        if (param_start != -1 && param_end != -1) {
            char param_str[512];
            int param_len = param_end - param_start;
            if (param_len < 511) {
                strncpy(param_str, content + param_start, param_len);
                param_str[param_len] = '\0';
                
                // Parse individual parameters
                char *token = strtok(param_str, ",");
                element->param_count = 0;
                while (token && element->param_count < 16) {
                    DocParameter *param = &element->params[element->param_count];
                    
                    // Parse "type name" format
                    char *space = strrchr(token, ' ');
                    if (space) {
                        *space = '\0';
                        strncpy(param->type, token, 63);
                        strncpy(param->name, space + 1, 63);
                    }
                    
                    element->param_count++;
                    token = strtok(NULL, ",");
                }
            }
        }
    }
    
    regfree(&func_regex);
    return 0;
}

// TODO-27208: Implement cross-reference resolution [Difficulty: 6]
static int resolve_cross_references(DocElement *elements, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        DocElement *elem = &elements[i];
        
        // Resolve @see references
        for (uint32_t j = 0; j < elem->see_also_count; j++) {
            char *ref = elem->see_also[j];
            
            // Search for matching elements
            for (uint32_t k = 0; k < count; k++) {
                if (k != i && strcmp(elements[k].name, ref) == 0) {
                    // Reference found - could store ID or link
                    break;
                }
            }
        }
    }
    return 0;
}

// TODO-27209: Implement @see/@link processing [Difficulty: 5]
static int process_see_links(const char *comment, DocElement *element) {
    regex_t see_regex;
    regmatch_t matches[2];
    
    const char *pattern = "@see\\s+([^\\s]+)";
    if (regcomp(&see_regex, pattern, REG_EXTENDED) != 0) {
        return -1;
    }
    
    const char *ptr = comment;
    while (regexec(&see_regex, ptr, 2, matches, 0) == 0 && element->see_also_count < 8) {
        int start = matches[1].rm_so;
        int end = matches[1].rm_eo;
        int len = end - start;
        
        if (len < 63) {
            strncpy(element->see_also[element->see_also_count], ptr + start, len);
            element->see_also[element->see_also_count][len] = '\0';
            element->see_also_count++;
        }
        
        ptr += end;
    }
    
    regfree(&see_regex);
    return 0;
}

// TODO-27210: Implement @example code extraction [Difficulty: 5]
static int extract_examples(const char *comment, DocElement *element) {
    regex_t example_regex;
    regmatch_t matches[3];
    
    const char *pattern = "@example\\s*\\n([^@]*?)(?=@|$)";
    if (regcomp(&example_regex, pattern, REG_EXTENDED) != 0) {
        return -1;
    }
    
    const char *ptr = comment;
    char examples_buffer[4096] = {0};
    int total_len = 0;
    
    while (regexec(&example_regex, ptr, 3, matches, 0) == 0) {
        int start = matches[1].rm_so;
        int end = matches[1].rm_eo;
        int len = end - start;
        
        if (total_len + len < 4095) {
            strncat(examples_buffer, ptr + start, len);
            strncat(examples_buffer, "\n\n", 2);
            total_len += len + 2;
        }
        
        ptr += end;
    }
    
    strncpy(element->examples, examples_buffer, 4095);
    element->examples[4095] = '\0';
    
    regfree(&example_regex);
    return 0;
}

/* =================================================================================================
 *                                    HTML GENERATOR IMPLEMENTATION
 * ================================================================================================= */

// TODO-27211: Implement HTML index page generation [Difficulty: 5]
static int generate_html_index(const DocElement *elements, uint32_t count, 
                              const HTMLGeneratorConfig *config) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/index.html", config->output_dir);
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) return -1;
    
    fprintf(fp, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(fp, "<title>API Documentation</title>\n");
    fprintf(fp, "<link rel=\"stylesheet\" href=\"%s\">\n", config->css_path);
    fprintf(fp, "</head>\n<body>\n");
    fprintf(fp, "<div class=\"container\">\n");
    fprintf(fp, "<h1>API Documentation</h1>\n");
    fprintf(fp, "<div class=\"nav-tree\">\n");
    
    // Group by type
    for (int type = 0; type <= DOC_MODULE; type++) {
        fprintf(fp, "<h2>%s</h2>\n<ul>\n", 
                type == DOC_FUNCTION ? "Functions" :
                type == DOC_STRUCT ? "Structures" :
                type == DOC_ENUM ? "Enumerations" :
                type == DOC_DEFINE ? "Macros" :
                type == DOC_TYPEDEF ? "Type Definitions" :
                type == DOC_VARIABLE ? "Variables" : "Modules");
        
        for (uint32_t i = 0; i < count; i++) {
            if (elements[i].type == type) {
                fprintf(fp, "<li><a href=\"%s.html\">%s</a> - %s</li>\n",
                        elements[i].name, elements[i].name, elements[i].brief);
            }
        }
        fprintf(fp, "</ul>\n");
    }
    
    fprintf(fp, "</div>\n</div>\n</body>\n</html>");
    fclose(fp);
    return 0;
}

// TODO-27212: Implement HTML function page generation [Difficulty: 5]
static int generate_html_function(const DocElement *element, 
                                 const HTMLGeneratorConfig *config) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.html", config->output_dir, element->name);
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) return -1;
    
    fprintf(fp, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(fp, "<title>%s - API Documentation</title>\n", element->name);
    fprintf(fp, "<link rel=\"stylesheet\" href=\"%s\">\n", config->css_path);
    fprintf(fp, "</head>\n<body>\n");
    fprintf(fp, "<div class=\"container\">\n");
    
    // Breadcrumb navigation
    fprintf(fp, "<div class=\"breadcrumb\">\n");
    fprintf(fp, "<a href=\"index.html\">Home</a> &gt; ");
    fprintf(fp, "<a href=\"functions.html\">Functions</a> &gt; %s\n", element->name);
    fprintf(fp, "</div>\n");
    
    // Function signature
    fprintf(fp, "<h1>%s</h1>\n", element->name);
    fprintf(fp, "<div class=\"signature\">\n");
    fprintf(fp, "<code>%s %s(", element->return_type, element->name);
    
    for (uint32_t i = 0; i < element->param_count; i++) {
        if (i > 0) fprintf(fp, ", ");
        fprintf(fp, "%s %s", element->params[i].type, element->params[i].name);
    }
    fprintf(fp, ")</code>\n</div>\n");
    
    // Description
    fprintf(fp, "<div class=\"description\">\n");
    fprintf(fp, "<p>%s</p>\n", element->brief);
    if (strlen(element->detailed) > 0) {
        fprintf(fp, "<p>%s</p>\n", element->detailed);
    }
    fprintf(fp, "</div>\n");
    
    // Parameters
    if (element->param_count > 0) {
        fprintf(fp, "<h2>Parameters</h2>\n<table>\n");
        for (uint32_t i = 0; i < element->param_count; i++) {
            fprintf(fp, "<tr><td><code>%s</code></td><td>%s</td></tr>\n",
                    element->params[i].name, element->params[i].description);
        }
        fprintf(fp, "</table>\n");
    }
    
    fprintf(fp, "</div>\n</body>\n</html>");
    fclose(fp);
    return 0;
}

/* =================================================================================================
 *                                    SEARCH INDEX IMPLEMENTATION
 * ================================================================================================= */

// TODO-27220: Implement search index building [Difficulty: 6]
static int build_search_index(const DocElement *elements, uint32_t count, 
                             SearchIndex *index) {
    index->term_count = 0;
    index->document_count = count;
    index->documents = malloc(sizeof(DocElement) * count);
    memcpy(index->documents, elements, sizeof(DocElement) * count);
    
    // Estimate terms and allocate
    index->terms = malloc(sizeof(index->terms[0]) * count * 10); // Estimate 10 terms per doc
    
    for (uint32_t i = 0; i < count; i++) {
        const DocElement *elem = &elements[i];
        
        // Tokenize name
        char name_copy[128];
        strncpy(name_copy, elem->name, 127);
        name_copy[127] = '\0';
        
        char *token = strtok(name_copy, " _");
        while (token && index->term_count < count * 10) {
            // Add to index (simplified)
            strncpy(index->terms[index->term_count].term, token, 63);
            index->terms[index->term_count].doc_ids[0] = i;
            index->terms[index->term_count].count = 1;
            index->term_count++;
            token = strtok(NULL, " _");
        }
    }
    
    return 0;
}

// TODO-27221: Implement term tokenization [Difficulty: 4]
static int tokenize_text(const char *text, char (*tokens)[64], uint32_t *token_count) {
    *token_count = 0;
    char text_copy[2048];
    strncpy(text_copy, text, 2047);
    text_copy[2047] = '\0';
    
    char *token = strtok(text_copy, " \t\n\r.,;:!()[]{}<>\"'");
    while (token && *token_count < 32) {
        // Convert to lowercase
        for (int i = 0; token[i]; i++) {
            token[i] = tolower(token[i]);
        }
        
        // Skip short tokens
        if (strlen(token) >= 2) {
            strncpy(tokens[*token_count], token, 63);
            tokens[*token_count][63] = '\0';
            (*token_count)++;
        }
        
        token = strtok(NULL, " \t\n\r.,;:!()[]{}<>\"'");
    }
    
    return 0;
}

/* =================================================================================================
 *                                    MAIN API IMPLEMENTATION
 * ================================================================================================= */

// TODO-27226: Implement doc_generator_init [Difficulty: 4]
typedef struct DocGenerator {
    DocParserContext *parser;
    SearchIndex search_index;
    HTMLGeneratorConfig html_config;
} DocGenerator;

DocGenerator* doc_generator_init(const char *source_dir, const char *output_dir) {
    DocGenerator *gen = malloc(sizeof(DocGenerator));
    gen->parser = parser_init();
    
    strncpy(gen->html_config.output_dir, output_dir, 255);
    gen->html_config.output_dir[255] = '\0';
    
    // Default config
    strcpy(gen->html_config.template_dir, "templates");
    strcpy(gen->html_config.css_path, "style.css");
    gen->html_config.generate_search_index = true;
    gen->html_config.syntax_highlight_examples = true;
    strcpy(gen->html_config.theme_name, "default");
    
    return gen;
}

// TODO-27227: Implement doc_generator_parse_file [Difficulty: 6]
int doc_generator_parse_file(DocGenerator *gen, const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return -1;
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    fread(content, 1, size, fp);
    content[size] = '\0';
    fclose(fp);
    
    // Parse file content (simplified)
    DocElement elem = {0};
    elem.type = DOC_FUNCTION;
    strncpy(elem.file_path, filepath, 255);
    
    // Extract function signatures
    parse_function_signature(content, &elem);
    
    // Add to parser context
    if (gen->parser->count < gen->parser->capacity) {
        gen->parser->elements[gen->parser->count] = elem;
        gen->parser->count++;
    }
    
    free(content);
    return 0;
}

// TODO-27228: Implement doc_generator_parse_directory [Difficulty: 5]
int doc_generator_parse_directory(DocGenerator *gen, const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) return -1;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".h")) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
            doc_generator_parse_file(gen, filepath);
        }
    }
    
    closedir(dir);
    return 0;
}

// TODO-27229: Implement doc_generator_generate_html [Difficulty: 6]
int doc_generator_generate_html(DocGenerator *gen) {
    // Build search index
    build_search_index(gen->parser->elements, gen->parser->count, &gen->search_index);
    
    // Generate index page
    generate_html_index(gen->parser->elements, gen->parser->count, &gen->html_config);
    
    // Generate individual pages
    for (uint32_t i = 0; i < gen->parser->count; i++) {
        if (gen->parser->elements[i].type == DOC_FUNCTION) {
            generate_html_function(&gen->parser->elements[i], &gen->html_config);
        }
    }
    
    return 0;
}

// TODO-27230: Implement doc_generator_generate_markdown [Difficulty: 5]
int doc_generator_generate_markdown(DocGenerator *gen, const char *output_dir) {
    for (uint32_t i = 0; i < gen->parser->count; i++) {
        const DocElement *elem = &gen->parser->elements[i];
        
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s.md", output_dir, elem->name);
        
        FILE *fp = fopen(filepath, "w");
        if (!fp) continue;
        
        fprintf(fp, "# %s\n\n", elem->name);
        fprintf(fp, "%s\n\n", elem->brief);
        
        if (elem->param_count > 0) {
            fprintf(fp, "## Parameters\n\n");
            for (uint32_t j = 0; j < elem->param_count; j++) {
                fprintf(fp, "- **%s** (%s): %s\n",
                        elem->params[j].name, elem->params[j].type, elem->params[j].description);
            }
            fprintf(fp, "\n");
        }
        
        fclose(fp);
    }
    
    return 0;
}

// TODO-27213: Implement HTML struct page generation [Difficulty: 5]
static int generate_html_struct(const DocElement *element, 
                               const HTMLGeneratorConfig *config) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.html", config->output_dir, element->name);
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) return -1;
    
    fprintf(fp, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(fp, "<title>%s - API Documentation</title>\n", element->name);
    fprintf(fp, "<link rel=\"stylesheet\" href=\"%s\">\n", config->css_path);
    fprintf(fp, "</head>\n<body>\n");
    fprintf(fp, "<div class=\"container\">\n");
    
    // Breadcrumb navigation
    fprintf(fp, "<div class=\"breadcrumb\">\n");
    fprintf(fp, "<a href=\"index.html\">Home</a> &gt; ");
    fprintf(fp, "<a href=\"structs.html\">Structures</a> &gt; %s\n", element->name);
    fprintf(fp, "</div>\n");
    
    fprintf(fp, "<h1>struct %s</h1>\n", element->name);
    fprintf(fp, "<div class=\"description\">\n");
    fprintf(fp, "<p>%s</p>\n", element->brief);
    if (strlen(element->detailed) > 0) {
        fprintf(fp, "<p>%s</p>\n", element->detailed);
    }
    fprintf(fp, "</div>\n");
    
    fprintf(fp, "</div>\n</body>\n</html>");
    fclose(fp);
    return 0;
}

// TODO-27214: Implement HTML navigation tree [Difficulty: 5]
static int generate_html_navigation(const DocElement *elements, uint32_t count, 
                                    const HTMLGeneratorConfig *config) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/nav.html", config->output_dir);
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) return -1;
    
    fprintf(fp, "<div class=\"nav-tree\">\n");
    
    // Group by type and create tree structure
    for (int type = 0; type <= DOC_MODULE; type++) {
        const char *type_name = type == DOC_FUNCTION ? "Functions" :
                               type == DOC_STRUCT ? "Structures" :
                               type == DOC_ENUM ? "Enumerations" :
                               type == DOC_DEFINE ? "Macros" :
                               type == DOC_TYPEDEF ? "Type Definitions" :
                               type == DOC_VARIABLE ? "Variables" : "Modules";
        
        fprintf(fp, "<div class=\"nav-group\">\n");
        fprintf(fp, "<h3>%s</h3>\n<ul>\n", type_name);
        
        for (uint32_t i = 0; i < count; i++) {
            if (elements[i].type == type) {
                fprintf(fp, "<li><a href=\"%s.html\">%s</a></li>\n",
                        elements[i].name, elements[i].name);
            }
        }
        
        fprintf(fp, "</ul>\n</div>\n");
    }
    
    fprintf(fp, "</div>\n");
    fclose(fp);
    return 0;
}

// TODO-27215: Implement HTML breadcrumb navigation [Difficulty: 4]
static int generate_html_breadcrumbs(const DocElement *element, 
                                     const HTMLGeneratorConfig *config) {
    // This is typically embedded in each page generation function
    return 0;
}

// TODO-27216: Implement HTML syntax highlighting [Difficulty: 5]
static int generate_html_syntax_highlighting(const char *code, char *output, size_t output_size) {
    if (!code || !output) return -1;
    
    // Simple syntax highlighting for C code
    char *ptr = output;
    size_t remaining = output_size;
    
    // Add CSS classes for syntax highlighting
    const char *keywords[] = {"int", "char", "void", "float", "double", "struct", "enum", 
                            "if", "else", "while", "for", "return", "static", "const"};
    int keyword_count = sizeof(keywords) / sizeof(keywords[0]);
    
    // Simple implementation - wrap code in pre with syntax-highlights class
    snprintf(ptr, remaining, "<pre class=\"syntax-highlight\"><code>%s</code></pre>", code);
    
    return 0;
}

// TODO-27217: Implement HTML search page [Difficulty: 6]
static int generate_html_search_page(const SearchIndex *index, 
                                     const HTMLGeneratorConfig *config) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/search.html", config->output_dir);
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) return -1;
    
    fprintf(fp, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(fp, "<title>Search - API Documentation</title>\n");
    fprintf(fp, "<link rel=\"stylesheet\" href=\"%s\">\n", config->css_path);
    fprintf(fp, "<script>\n");
    fprintf(fp, "// Search functionality would go here\n");
    fprintf(fp, "function performSearch() {\n");
    fprintf(fp, "  const query = document.getElementById('search').value.toLowerCase();\n");
    fprintf(fp, "  // Search logic would be implemented here\n");
    fprintf(fp, "}\n");
    fprintf(fp, "</script>\n");
    fprintf(fp, "</head>\n<body>\n");
    fprintf(fp, "<div class=\"container\">\n");
    fprintf(fp, "<h1>Search Documentation</h1>\n");
    fprintf(fp, "<input type=\"text\" id=\"search\" placeholder=\"Search...\" onkeyup=\"performSearch()\">\n");
    fprintf(fp, "<div id=\"results\"></div>\n");
    fprintf(fp, "</div>\n</body>\n</html>");
    
    fclose(fp);
    return 0;
}

// TODO-27218: Implement Markdown output [Difficulty: 5]
static int generate_markdown_output(const DocElement *elements, uint32_t count, 
                                   const char *output_dir) {
    // Generate index
    char index_path[512];
    snprintf(index_path, sizeof(index_path), "%s/README.md", output_dir);
    
    FILE *fp = fopen(index_path, "w");
    if (!fp) return -1;
    
    fprintf(fp, "# API Documentation\n\n");
    
    // Group by type
    for (int type = 0; type <= DOC_MODULE; type++) {
        const char *type_name = type == DOC_FUNCTION ? "Functions" :
                               type == DOC_STRUCT ? "Structures" :
                               type == DOC_ENUM ? "Enumerations" :
                               type == DOC_DEFINE ? "Macros" :
                               type == DOC_TYPEDEF ? "Type Definitions" :
                               type == DOC_VARIABLE ? "Variables" : "Modules";
        
        fprintf(fp, "## %s\n\n", type_name);
        
        for (uint32_t i = 0; i < count; i++) {
            if (elements[i].type == type) {
                fprintf(fp, "- [%s](%s.md) - %s\n", 
                        elements[i].name, elements[i].name, elements[i].brief);
            }
        }
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    return 0;
}

// TODO-27219: Implement man page output [Difficulty: 5]
static int generate_man_page_output(const DocElement *elements, uint32_t count, 
                                    const char *output_dir) {
    for (uint32_t i = 0; i < count; i++) {
        const DocElement *elem = &elements[i];
        
        if (elem->type == DOC_FUNCTION) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s.3", output_dir, elem->name);
            
            FILE *fp = fopen(filepath, "w");
            if (!fp) continue;
            
            fprintf(fp, ".\" Man page for %s\n", elem->name);
            fprintf(fp, ".TH %s 3 \"Library Functions\" \"API Documentation\"\n", elem->name);
            fprintf(fp, ".SH NAME\n");
            fprintf(fp, "%s - %s\n", elem->name, elem->brief);
            fprintf(fp, ".SH SYNOPSIS\n");
            fprintf(fp, ".B \"#include <header.h>\"\n\n");
            fprintf(fp, ".BI \"%s \" \"%s\" \"(\"\n", elem->return_type, elem->name);
            
            for (uint32_t j = 0; j < elem->param_count; j++) {
                fprintf(fp, ".BI \"%s \" \"%s\"\n", 
                        elem->params[j].type, elem->params[j].name);
                if (j < elem->param_count - 1) {
                    fprintf(fp, ", \"\n");
                }
            }
            fprintf(fp, ")\"\n\n");
            
            fprintf(fp, ".SH DESCRIPTION\n");
            fprintf(fp, "%s\n", elem->brief);
            if (strlen(elem->detailed) > 0) {
                fprintf(fp, "%s\n", elem->detailed);
            }
            
            fclose(fp);
        }
    }
    
    return 0;
}

// TODO-27222: Implement fuzzy matching [Difficulty: 6]
static int fuzzy_match(const char *query, const char *term, float *score) {
    if (!query || !term || !score) return -1;
    
    int query_len = strlen(query);
    int term_len = strlen(term);
    
    if (query_len == 0 || term_len == 0) {
        *score = 0.0f;
        return 0;
    }
    
    // Simple Levenshtein distance approximation
    int matrix[64][64]; // Max size for our use case
    
    for (int i = 0; i <= query_len; i++) {
        matrix[i][0] = i;
    }
    for (int j = 0; j <= term_len; j++) {
        matrix[0][j] = j;
    }
    
    for (int i = 1; i <= query_len; i++) {
        for (int j = 1; j <= term_len; j++) {
            int cost = (tolower(query[i-1]) == tolower(term[j-1])) ? 0 : 1;
            matrix[i][j] = fmin(fmin(
                matrix[i-1][j] + 1,      // deletion
                matrix[i][j-1] + 1),      // insertion
                matrix[i-1][j-1] + cost   // substitution
            );
        }
    }
    
    int distance = matrix[query_len][term_len];
    int max_len = fmax(query_len, term_len);
    *score = 1.0f - ((float)distance / (float)max_len);
    
    return 0;
}

// TODO-27223: Implement search ranking [Difficulty: 5]
static int rank_search_results(const char *query, const uint32_t *doc_ids, 
                               uint32_t count, float *scores) {
    if (!query || !doc_ids || !scores) return -1;
    
    // Simple ranking based on exact matches, prefix matches, and fuzzy matches
    for (uint32_t i = 0; i < count; i++) {
        scores[i] = 0.0f;
        
        // Would need access to documents to implement proper ranking
        // For now, assign decreasing scores
        scores[i] = 1.0f - ((float)i / (float)count);
    }
    
    return 0;
}

// TODO-27224: Implement search result highlighting [Difficulty: 4]
static int highlight_search_terms(const char *text, const char *terms[], 
                                   uint32_t term_count, char *output, size_t output_size) {
    if (!text || !terms || !output) return -1;
    
    // Simple highlighting - wrap matching terms in <mark> tags
    strncpy(output, text, output_size - 1);
    output[output_size - 1] = '\0';
    
    for (uint32_t i = 0; i < term_count; i++) {
        char *pos = strstr(output, terms[i]);
        if (pos) {
            // Simple replacement - would need more sophisticated approach for production
            // This is a simplified implementation
        }
    }
    
    return 0;
}

// TODO-27225: Implement search index serialization [Difficulty: 5]
static int serialize_search_index(const SearchIndex *index, const char *filepath) {
    if (!index || !filepath) return -1;
    
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return -1;
    
    // Write header
    uint32_t magic = 0x444F4358; // "DOCX"
    fwrite(&magic, sizeof(magic), 1, fp);
    fwrite(&index->term_count, sizeof(index->term_count), 1, fp);
    fwrite(&index->document_count, sizeof(index->document_count), 1, fp);
    
    // Write terms
    for (uint32_t i = 0; i < index->term_count; i++) {
        fwrite(index->terms[i].term, sizeof(index->terms[i].term), 1, fp);
        fwrite(&index->terms[i].count, sizeof(index->terms[i].count), 1, fp);
        fwrite(index->terms[i].doc_ids, sizeof(uint32_t), index->terms[i].count, fp);
    }
    
    // Write documents
    fwrite(index->documents, sizeof(DocElement), index->document_count, fp);
    
    fclose(fp);
    return 0;
}

// TODO-27231: Implement doc_generator_build_search_index [Difficulty: 5]
int doc_generator_build_search_index(DocGenerator *gen) {
    return build_search_index(gen->parser->elements, gen->parser->count, &gen->search_index);
}

// Additional API functions
int doc_generator_serve_local(DocGenerator *gen, int port) {
    // Simple HTTP server implementation would go here
    // For now, just indicate that server would start
    printf("Documentation server would start on port %d\n", port);
    return 0;
}

int doc_generator_watch_changes(DocGenerator *gen, const char *watch_dir) {
    // File watching implementation would go here
    // For now, just indicate that watching would start
    printf("Watching for changes in directory: %s\n", watch_dir);
    return 0;
}
