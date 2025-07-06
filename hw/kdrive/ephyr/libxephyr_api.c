#include "dix/context.h"
/*
 * libxephyr - Xephyr as a shared library
 * API implementation for embedding Xephyr X server instances
 *
 * Copyright © 2025 Two-Tiles Project
 */

#define _GNU_SOURCE

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
#include <time.h>
#include <errno.h>

/* Internal server structure */
struct XephyrServer {
    XephyrConfig config;
    pid_t server_pid;
    Window embedded_window;
    char display_name[32];
    char title[64];
    bool running;
    bool ready;
    pthread_t server_thread;
    pthread_mutex_t mutex;
    pthread_cond_t ready_cond;
    int argc;
    char** argv;
};

/* Global state */
static bool library_initialized = false;
static int server_counter = 0;
static XephyrServer* current_server = NULL;

/* Forward declarations */
static void* xephyr_server_thread(void* arg);
static int build_argv(XephyrServer* server);
static void cleanup_argv(XephyrServer* server);

/* Error strings */
static const char* error_strings[] = {
    "Success",
    "Invalid configuration",
    "Failed to open xephyr_context->display",
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
    
    fprintf(stderr, "DEBUG: Before copy - config->width=%d, config->height=%d\n", 
            config->width, config->height);
    
    server->config = *config;
    
    fprintf(stderr, "DEBUG: After copy - server->config.width=%d, server->config.height=%d\n", 
            server->config.width, server->config.height);
    
    server->server_pid = -1;
    server->running = false;
    server->ready = false;
    
    /* Initialize synchronization primitives */
    if (pthread_mutex_init(&server->mutex, NULL) != 0) {
        free(server);
        return NULL;
    }
    if (pthread_cond_init(&server->ready_cond, NULL) != 0) {
        pthread_mutex_destroy(&server->mutex);
        free(server);
        return NULL;
    }
    
    /* Generate unique xephyr_context->display name if not provided */
    if (!config->display_name) {
        snprintf(server->display_name, sizeof(server->display_name), ":%d", 100 + server_counter++);
    } else {
        strncpy(server->display_name, config->display_name, sizeof(server->display_name) - 1);
        server->display_name[sizeof(server->display_name) - 1] = '\0';
    }
    server->config.display_name = server->display_name;
    
    /* Copy title if provided */
    if (config->title) {
        strncpy(server->title, config->title, sizeof(server->title) - 1);
        server->title[sizeof(server->title) - 1] = '\0';
        server->config.title = server->title;
    } else {
        strcpy(server->title, "Xephyr");
        server->config.title = server->title;
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
    pthread_mutex_destroy(&server->mutex);
    pthread_cond_destroy(&server->ready_cond);
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
    server->argv[server->argc++] = strdup(server->display_name);
    
    /* Parent window FIRST - before -screen */
    if (server->config.parent_window) {
        char parent_str[32];
        snprintf(parent_str, sizeof(parent_str), "%lu", server->config.parent_window);
        server->argv[server->argc++] = strdup("-parent");
        server->argv[server->argc++] = strdup(parent_str);
    }
    
    /* Screen size AFTER parent */
    char screen_size[64];
    fprintf(stderr, "DEBUG: config.width=%d, config.height=%d\n", 
            server->config.width, server->config.height);
    snprintf(screen_size, sizeof(screen_size), "%dx%d", 
             server->config.width, server->config.height);
    fprintf(stderr, "DEBUG: screen_size='%s'\n", screen_size);
    server->argv[server->argc++] = strdup("-screen");
    server->argv[server->argc++] = strdup(screen_size);
    
    /* Additional options */
    if (server->config.resizable) {
        server->argv[server->argc++] = strdup("-resizeable");
    }
    
    if (server->config.fullscreen) {
        server->argv[server->argc++] = strdup("-fullscreen");
    }
    
    if (server->config.use_glamor) {
        server->argv[server->argc++] = strdup("-glamor");
        server->argv[server->argc++] = strdup("-iglx");
        server->argv[server->argc++] = strdup("-noreset");
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
        pthread_mutex_lock(&server->mutex);
        server->running = false;
        server->ready = false;
        pthread_cond_signal(&server->ready_cond);
        pthread_mutex_unlock(&server->mutex);
        return NULL;
    }
    
    /* Mark as running and set as current server */
    pthread_mutex_lock(&server->mutex);
    server->running = true;
    current_server = server;
    pthread_mutex_unlock(&server->mutex);
    
    /* Call the actual X server main function directly */
    extern int dix_main(int argc, char *argv[], char *envp[]);
    extern char **environ;
    
    /* Debug: print arguments */
    fprintf(stderr, "Starting Xephyr with args: ");
    for (int i = 0; i < server->argc; i++) {
        fprintf(stderr, "'%s' ", server->argv[i]);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    
    /* Call real X server - this blocks until server exits */
    int result = dix_main(server->argc, server->argv, environ);
    
    /* Server finished - signal waiting thread */
    pthread_mutex_lock(&server->mutex);
    server->running = false;
    server->ready = false;
    pthread_cond_signal(&server->ready_cond); /* Signal in case main thread is still waiting */
    pthread_mutex_unlock(&server->mutex);
    
    cleanup_argv(server);
    return NULL;
}

/* Function to be called from X server when ready */
void xephyr_signal_ready(void) {
    fprintf(stderr, "DEBUG: xephyr_signal_ready called\n");
    fflush(stderr);
    if (current_server) {
        pthread_mutex_lock(&current_server->mutex);
        current_server->ready = true;
        pthread_cond_signal(&current_server->ready_cond);
        pthread_mutex_unlock(&current_server->mutex);
        fprintf(stderr, "DEBUG: xephyr_signal_ready completed\n");
        fflush(stderr);
    } else {
        fprintf(stderr, "DEBUG: current_server is NULL\n");
        fflush(stderr);
    }
}

XephyrError xephyr_server_start(XephyrServer* server) {
    if (!server) {
        return XEPHYR_ERROR_NULL_POINTER;
    }
    
    pthread_mutex_lock(&server->mutex);
    if (server->running) {
        pthread_mutex_unlock(&server->mutex);
        return XEPHYR_SUCCESS; /* Already running */
    }
    pthread_mutex_unlock(&server->mutex);
    
    /* Create server thread */
    if (pthread_create(&server->server_thread, NULL, xephyr_server_thread, server) != 0) {
        return XEPHYR_ERROR_SERVER_START;
    }
    
    /* Wait for server to be ready or fail with timeout */
    pthread_mutex_lock(&server->mutex);
    
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 10; // 10 second timeout
    
    /* Wait until either ready or timeout */
    int wait_result = 0;
    while (!server->ready && wait_result == 0) {
        wait_result = pthread_cond_timedwait(&server->ready_cond, &server->mutex, &timeout);
    }
    
    bool ready = server->ready;
    bool running = server->running;
    pthread_mutex_unlock(&server->mutex);
    
    /* If server failed to start, wait for thread to finish before returning error */
    if (!ready && !running) {
        pthread_join(server->server_thread, NULL);
    }
    
    return ready ? XEPHYR_SUCCESS : XEPHYR_ERROR_SERVER_START;
}

XephyrError xephyr_server_stop(XephyrServer* server) {
    if (!server) {
        return XEPHYR_ERROR_NULL_POINTER;
    }
    
    pthread_mutex_lock(&server->mutex);
    if (!server->running) {
        pthread_mutex_unlock(&server->mutex);
        return XEPHYR_SUCCESS; /* Already stopped */
    }
    
    server->running = false;
    pthread_mutex_unlock(&server->mutex);
    
    /* Force server shutdown by setting dispatchException */
    extern volatile char dispatchException;
    extern void AbortServer(void);
    
    /* Signal server to exit gracefully */
    dispatchException = 1; /* DE_TERMINATE */
    
    /* Give it a moment to exit gracefully */
    struct timespec wait_time = {0, 100000000}; /* 100ms */
    nanosleep(&wait_time, NULL);
    
    /* Wait for thread to finish with timeout */
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 2; /* 2 second timeout */
    
    int result = pthread_timedjoin_np(server->server_thread, NULL, &timeout);
    if (result == ETIMEDOUT) {
        /* Force termination if thread doesn't exit gracefully */
        pthread_cancel(server->server_thread);
        pthread_join(server->server_thread, NULL);
    }
    
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
    return server->display_name;
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