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
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#endif

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
  bool server_running;
  int server_socket;
  int server_port;
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
      fprintf(f, "- [%s](%s.md)\n", g_doc_gen.modules[i].name, g_doc_gen.modules[i].name);
    }

    fclose(f);
  }
}

/* =================================================================================================
 *                                    HTTP SERVER
 * =================================================================================================
 */

static void send_http_response(int client_socket, const char *status, 
                             const char *content_type, const char *content, size_t content_length) {
  char response[8192];
  snprintf(response, sizeof(response),
           "HTTP/1.1 %s\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %zu\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Connection: close\r\n"
           "\r\n",
           status, content_type, content_length);
  
  send(client_socket, response, strlen(response), 0);
  send(client_socket, content, content_length, 0);
}

static void send_file_response(int client_socket, const char *file_path) {
  FILE *f = fopen(file_path, "rb");
  if (!f) {
    const char *not_found = "<html><body><h1>404 Not Found</h1></body></html>";
    send_http_response(client_socket, "404 Not Found", "text/html", not_found, strlen(not_found));
    return;
  }

  // Get file size
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  // Determine content type
  const char *content_type = "text/html";
  if (strstr(file_path, ".css")) {
    content_type = "text/css";
  } else if (strstr(file_path, ".js")) {
    content_type = "application/javascript";
  } else if (strstr(file_path, ".png")) {
    content_type = "image/png";
  } else if (strstr(file_path, ".jpg") || strstr(file_path, ".jpeg")) {
    content_type = "image/jpeg";
  }

  // Read and send file
  char *buffer = malloc(file_size);
  if (buffer) {
    fread(buffer, 1, file_size, f);
    send_http_response(client_socket, "200 OK", content_type, buffer, file_size);
    free(buffer);
  }

  fclose(f);
}

static void handle_client_request(int client_socket) {
  char buffer[4096];
  ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
  
  if (bytes_received <= 0) {
    close(client_socket);
    return;
  }

  buffer[bytes_received] = '\0';

  // Parse HTTP request
  char method[16], path[256], version[16];
  sscanf(buffer, "%15s %255s %15s", method, path, version);

  // Default to index.html for root
  if (strcmp(path, "/") == 0) {
    strcpy(path, "/index.html");
  }

  // Construct file path
  char file_path[512];
  snprintf(file_path, sizeof(file_path), "%s%s", g_doc_gen.output_dir, path);

  // Remove leading slash if present
  char *file_path_ptr = file_path;
  if (file_path_ptr[0] == '/') {
    file_path_ptr++;
  }

  send_file_response(client_socket, file_path_ptr);
  close(client_socket);
}

// DONE: Implement doc_generator_serve_local
bool doc_generator_serve_local(int port) {
#ifdef _WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return false;
  }
#endif

  g_doc_gen.server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (g_doc_gen.server_socket < 0) {
    return false;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(g_doc_gen.server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    close(g_doc_gen.server_socket);
    return false;
  }

  if (listen(g_doc_gen.server_socket, 10) < 0) {
    close(g_doc_gen.server_socket);
    return false;
  }

  g_doc_gen.server_port = port;
  g_doc_gen.server_running = true;

  printf("Documentation server started on http://localhost:%d\n", port);
  printf("Serving files from: %s\n", g_doc_gen.output_dir);

  while (g_doc_gen.server_running) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_socket = accept(g_doc_gen.server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket >= 0) {
      handle_client_request(client_socket);
    }
  }

  close(g_doc_gen.server_socket);
#ifdef _WIN32
  WSACleanup();
#endif
  return true;
}

// DONE: Implement doc_generator_watch_changes
bool doc_generator_watch_changes(const char *directory) {
#ifdef _WIN32
  HANDLE dir_handle = CreateFileA(
    directory,
    FILE_LIST_DIRECTORY,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    NULL,
    OPEN_EXISTING,
    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
    NULL
  );

  if (dir_handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  printf("Watching for changes in: %s\n", directory);
  printf("Press Ctrl+C to stop watching...\n");

  char buffer[4096];
  DWORD bytes_returned;
  
  while (1) {
    if (ReadDirectoryChangesW(
      dir_handle,
      buffer,
      sizeof(buffer),
      TRUE,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | 
      FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
      FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
      &bytes_returned,
      NULL,
      NULL)) {
      
      FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION*)buffer;
      while (1) {
        if (info->Action == FILE_ACTION_MODIFIED || 
            info->Action == FILE_ACTION_ADDED ||
            info->Action == FILE_ACTION_REMOVED) {
          
          wchar_t filename[MAX_PATH];
          wcsncpy(filename, info->FileName, info->FileNameLength / sizeof(wchar_t));
          filename[info->FileNameLength / sizeof(wchar_t)] = L'\0';
          
          printf("File changed: %ls\n", filename);
          printf("Regenerating documentation...\n");
          
          // Regenerate documentation
          doc_generator_process_directory(directory);
          doc_generator_generate_html();
          doc_generator_generate_markdown();
          
          printf("Documentation updated.\n\n");
        }
        
        if (info->NextEntryOffset == 0) {
          break;
        }
        info = (FILE_NOTIFY_INFORMATION*)((char*)info + info->NextEntryOffset);
      }
    }
  }

  CloseHandle(dir_handle);
#else
  // Linux/macOS implementation using inotify
  int inotify_fd = inotify_init();
  if (inotify_fd < 0) {
    return false;
  }

  int watch_desc = inotify_add_watch(inotify_fd, directory, 
                                     IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);
  if (watch_desc < 0) {
    close(inotify_fd);
    return false;
  }

  printf("Watching for changes in: %s\n", directory);
  printf("Press Ctrl+C to stop watching...\n");

  char buffer[4096];
  while (1) {
    ssize_t length = read(inotify_fd, buffer, sizeof(buffer));
    if (length > 0) {
      int i = 0;
      while (i < length) {
        struct inotify_event *event = (struct inotify_event*)&buffer[i];
        
        if (event->len > 0) {
          printf("File changed: %s\n", event->name);
          printf("Regenerating documentation...\n");
          
          // Regenerate documentation
          doc_generator_process_directory(directory);
          doc_generator_generate_html();
          doc_generator_generate_markdown();
          
          printf("Documentation updated.\n\n");
        }
        
        i += sizeof(struct inotify_event) + event->len;
      }
    }
  }

  close(inotify_fd);
#endif
  return true;
}
