/*
 * libxephyr - Xephyr as a shared library
 * API implementation for embedding Xephyr X server instances
 *
 * Copyright © 2025 Two-Tiles Project
 */

// Override visibility for our API functions
#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif

#include "libxephyr.h"
#include "ephyr.h"
#include "hostx.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

/* Internal server structure */
struct XephyrServer {
    XephyrConfig config;
    pid_t server_pid;
    Window embedded_window;
    char display_name[32];
    bool running;
    pthread_t server_thread;
    int argc;
    char** argv;
};

/* Global state */
static bool library_initialized = false;
static int server_counter = 0;

/* Forward declarations */
static void* xephyr_server_thread(void* arg);
static int build_argv(XephyrServer* server);
static void cleanup_argv(XephyrServer* server);

/* Error strings */
static const char* error_strings[] = {
    "Success",
    "Invalid configuration",
    "Failed to open display",
    "Failed to create window",
    "Failed to start server",
    "Failed to stop server",
    "Null pointer error"
};

const char* xephyr_error_string(XephyrError error) {
    int index = -error;
    if (index >= 0 && index < (int)(sizeof(error_strings) / sizeof(error_strings[0]))) {
        return error_strings[index];
    }
    return "Unknown error";
}

XephyrError xephyr_init(void) {
    if (library_initialized) {
        return XEPHYR_SUCCESS;
    }
    
    /* Initialize any global state */
    library_initialized = true;
    server_counter = 0;
    
    return XEPHYR_SUCCESS;
}

void xephyr_cleanup(void) {
    library_initialized = false;
    server_counter = 0;
}

XephyrConfig xephyr_config_default(void) {
    XephyrConfig config = {
        .width = 640,
        .height = 480,
        .depth = 24,
        .parent_window = 0,
        .display_name = NULL,
        .use_glamor = false,
        .resizable = false,
        .fullscreen = false,
        .title = "Xephyr"
    };
    return config;
}

XephyrServer* xephyr_server_create(const XephyrConfig* config) {
    if (!config) {
        return NULL;
    }
    
    if (!library_initialized) {
        xephyr_init();
    }
    
    XephyrServer* server = malloc(sizeof(XephyrServer));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(XephyrServer));
    server->config = *config;
    server->server_pid = -1;
    server->running = false;
    
    /* Generate unique display name if not provided */
    if (!config->display_name) {
        snprintf(server->display_name, sizeof(server->display_name), ":%d", 100 + server_counter++);
        server->config.display_name = server->display_name;
    } else {
        strncpy(server->display_name, config->display_name, sizeof(server->display_name) - 1);
    }
    
    return server;
}

void xephyr_server_destroy(XephyrServer* server) {
    if (!server) {
        return;
    }
    
    if (server->running) {
        xephyr_server_stop(server);
    }
    
    cleanup_argv(server);
    free(server);
}

static int build_argv(XephyrServer* server) {
    /* Build argument list for Xephyr */
    int max_args = 20;
    server->argv = malloc(max_args * sizeof(char*));
    if (!server->argv) {
        return -1;
    }
    
    server->argc = 0;
    
    /* Program name */
    server->argv[server->argc++] = strdup("Xephyr");
    
    /* Display name */
    server->argv[server->argc++] = strdup(server->config.display_name);
    
    /* Screen size */
    char screen_size[64];
    snprintf(screen_size, sizeof(screen_size), "%dx%d", 
             server->config.width, server->config.height);
    server->argv[server->argc++] = strdup("-screen");
    server->argv[server->argc++] = strdup(screen_size);
    
    /* Parent window */
    if (server->config.parent_window) {
        char parent_str[32];
        snprintf(parent_str, sizeof(parent_str), "%lu", server->config.parent_window);
        server->argv[server->argc++] = strdup("-parent");
        server->argv[server->argc++] = strdup(parent_str);
    }
    
    /* Additional options */
    if (server->config.resizable) {
        server->argv[server->argc++] = strdup("-resizeable");
    }
    
    if (server->config.fullscreen) {
        server->argv[server->argc++] = strdup("-fullscreen");
    }
    
    if (server->config.use_glamor) {
        server->argv[server->argc++] = strdup("-glamor");
    }
    
    if (server->config.title) {
        server->argv[server->argc++] = strdup("-title");
        server->argv[server->argc++] = strdup(server->config.title);
    }
    
    /* Terminate argv */
    server->argv[server->argc] = NULL;
    
    return 0;
}

static void cleanup_argv(XephyrServer* server) {
    if (server->argv) {
        for (int i = 0; i < server->argc; i++) {
            free(server->argv[i]);
        }
        free(server->argv);
        server->argv = NULL;
        server->argc = 0;
    }
}

/* Real implementation using actual X server functions */
static void* xephyr_server_thread(void* arg) {
    XephyrServer* server = (XephyrServer*)arg;
    
    if (build_argv(server) < 0) {
        server->running = false;
        return NULL;
    }
    
    /* Mark as running */
    server->running = true;
    
    /* Call the actual X server main function directly */
    extern int dix_main(int argc, char *argv[], char *envp[]);
    extern char **environ;
    
    /* Call real X server - this should work for single instance */
    int result = dix_main(server->argc, server->argv, environ);
    
    /* Server finished */
    server->running = false;
    
    cleanup_argv(server);
    return NULL;
}

XephyrError xephyr_server_start(XephyrServer* server) {
    if (!server) {
        return XEPHYR_ERROR_NULL_POINTER;
    }
    
    if (server->running) {
        return XEPHYR_SUCCESS; /* Already running */
    }
    
    /* Create server thread */
    if (pthread_create(&server->server_thread, NULL, xephyr_server_thread, server) != 0) {
        return XEPHYR_ERROR_SERVER_START;
    }
    
    /* Wait a bit for server to start */
    usleep(100000); /* 100ms */
    
    return server->running ? XEPHYR_SUCCESS : XEPHYR_ERROR_SERVER_START;
}

XephyrError xephyr_server_stop(XephyrServer* server) {
    if (!server) {
        return XEPHYR_ERROR_NULL_POINTER;
    }
    
    if (!server->running) {
        return XEPHYR_SUCCESS; /* Already stopped */
    }
    
    server->running = false;
    
    /* Wait for thread to finish */
    pthread_join(server->server_thread, NULL);
    
    return XEPHYR_SUCCESS;
}

Window xephyr_server_get_window(XephyrServer* server) {
    if (!server) {
        return 0;
    }
    return server->embedded_window;
}

const char* xephyr_server_get_display(XephyrServer* server) {
    if (!server) {
        return NULL;
    }
    return server->config.display_name;
}

bool xephyr_server_is_running(XephyrServer* server) {
    if (!server) {
        return false;
    }
    return server->running;
}

XephyrError xephyr_server_resize(XephyrServer* server, int width, int height) {
    if (!server) {
        return XEPHYR_ERROR_NULL_POINTER;
    }
    
    server->config.width = width;
    server->config.height = height;
    
    /* In a real implementation, we would send resize event to the server */
    
    return XEPHYR_SUCCESS;
}

XephyrError xephyr_server_set_title(XephyrServer* server, const char* title) {
    if (!server) {
        return XEPHYR_ERROR_NULL_POINTER;
    }
    
    /* In a real implementation, we would update the window title */
    server->config.title = title;
    
    return XEPHYR_SUCCESS;
}

XephyrError xephyr_server_process_events(XephyrServer* server) {
    if (!server) {
        return XEPHYR_ERROR_NULL_POINTER;
    }
    
    /* In a real implementation, we would process X11 events */
    
    return XEPHYR_SUCCESS;
}

#ifdef __GNUC__
#pragma GCC visibility pop
#endif