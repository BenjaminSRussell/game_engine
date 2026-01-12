#include <stdio.h>

void show_help() {
    printf("Localization Tool\n");
    printf("Usage: localization_tool [command]\n\n");
    printf("Commands:\n");
    printf("  --help\tShow this help message\n");
    printf("  --add\t\tAdd a new string\n");
    printf("  --edit\tEdit an existing string\n");
    printf("  --list\tList all strings\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        show_help();
    } else if (strcmp(argv[1], "--add") == 0) {
        printf("Adding a new string...\n");
        // Placeholder for adding a string
    } else if (strcmp(argv[1], "--edit") == 0) {
        printf("Editing a string...\n");
        // Placeholder for editing a string
    } else if (strcmp(argv[1], "--list") == 0) {
        printf("Listing all strings...\n");
        // Placeholder for listing strings
    } else {
        printf("Unknown command: %s\n", argv[1]);
        show_help();
        return 1;
    }

    return 0;
}
