/*
 * libxephyr - Xephyr as a shared library
 * API header for embedding Xephyr X server instances
 *
 * Copyright © 2025 Two-Tiles Project
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee.
 */

#ifndef LIBXEPHYR_H
#define LIBXEPHYR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations to avoid conflicts */
typedef unsigned long Window;
typedef unsigned long Display;

/* Xephyr server handle */
typedef struct XephyrServer XephyrServer;

/* Xephyr server configuration */
typedef struct {
    int width;
    int height;
    int depth;
    Window parent_window;  /* Parent X11 window to embed into */
    const char* display_name;  /* Display name (e.g., ":1") */
    bool use_glamor;       /* Enable hardware acceleration */
    bool resizable;        /* Allow window resizing */
    bool fullscreen;       /* Start in fullscreen mode */
    const char* title;     /* Window title */
} XephyrConfig;

/* Error codes */
typedef enum {
    XEPHYR_SUCCESS = 0,
    XEPHYR_ERROR_INVALID_CONFIG = -1,
    XEPHYR_ERROR_DISPLAY_OPEN = -2,
    XEPHYR_ERROR_WINDOW_CREATE = -3,
    XEPHYR_ERROR_SERVER_START = -4,
    XEPHYR_ERROR_SERVER_STOP = -5,
    XEPHYR_ERROR_NULL_POINTER = -6
} XephyrError;

/* Initialize library */
XephyrError xephyr_init(void);

/* Create a new Xephyr server instance */
XephyrServer* xephyr_server_create(const XephyrConfig* config);

/* Start the Xephyr server */
XephyrError xephyr_server_start(XephyrServer* server);

/* Stop the Xephyr server */
XephyrError xephyr_server_stop(XephyrServer* server);

/* Destroy Xephyr server instance */
void xephyr_server_destroy(XephyrServer* server);

/* Get the embedded window ID */
Window xephyr_server_get_window(XephyrServer* server);

/* Get the display name */
const char* xephyr_server_get_display(XephyrServer* server);

/* Check if server is running */
bool xephyr_server_is_running(XephyrServer* server);

/* Resize the embedded server window */
XephyrError xephyr_server_resize(XephyrServer* server, int width, int height);

/* Set window title */
XephyrError xephyr_server_set_title(XephyrServer* server, const char* title);

/* Process X11 events (call periodically) */
XephyrError xephyr_server_process_events(XephyrServer* server);

/* Cleanup library */
void xephyr_cleanup(void);

/* Utility function to create default config */
XephyrConfig xephyr_config_default(void);

/* Get error string for error code */
const char* xephyr_error_string(XephyrError error);

#ifdef __cplusplus
}
#endif

#endif /* LIBXEPHYR_H */