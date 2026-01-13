#ifndef LSP_SERVER_H
#define LSP_SERVER_H

void lsp_server_init(void);
void lsp_server_start(int port);
void lsp_server_handle_request(void *request, void *response);

#endif // LSP_SERVER_H
