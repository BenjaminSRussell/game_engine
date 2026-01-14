#ifndef CONTENT_MANAGER_H
#define CONTENT_MANAGER_H

#include <stdbool.h>

typedef enum {
    CONTENT_TYPE_ITEM,
    CONTENT_TYPE_QUEST,
    CONTENT_TYPE_DIALOGUE
} ContentType;

typedef struct {
    ContentType type;
    const char* prompt;
    int max_tokens;
} ContentRequest;

typedef struct {
    bool success;
    void* data;
} ContentResult;

typedef struct ContentManager ContentManager;

ContentManager* content_manager_create(void);
void content_manager_destroy(ContentManager* mgr);
ContentResult* content_manager_generate(ContentManager* mgr, const ContentRequest* req);
void content_result_destroy(ContentResult* res);

#endif // CONTENT_MANAGER_H
