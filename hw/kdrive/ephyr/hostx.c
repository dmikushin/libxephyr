#include "dix/context.h"
/*
 * Xephyr - A kdrive X server that runs in a host X window.
 *          Authored by Matthew Allum <mallum@o-hand.com>
 *
 * Copyright © 2004 Nokia
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Nokia not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Nokia makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * NOKIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL NOKIA BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "hostx.h"
#include "input.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>             /* for memset */
#include <errno.h>
#include <time.h>
#include <err.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_aux.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
#include <xcb/shape.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/randr.h>
#include <xcb/xkb.h>
#ifdef GLAMOR
#include <epoxy/gl.h>
#include "glamor.h"
#include "ephyr_glamor_glx.h"
#endif
#include "ephyrlog.h"
#include "ephyr.h"

/* struct EphyrHostXVars is defined in dix/context.h */

/* memset ( missing> ) instead of below  */
/*static EphyrHostXVars context->HostX = { "?", 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};*/
/* static EphyrHostXVars context->HostX; */

/* static int context->HostXWantDamageDebug = 0; */

// char *ephyrResName = NULL; - moved to XephyrContext
// int ephyrResNameFromCmd = 0; - moved to XephyrContext
// char *ephyrTitle = NULL; - moved to XephyrContext

Bool
hostx_has_extension(xcb_extension_t *extension, XephyrContext* context)
{
    const xcb_query_extension_reply_t *rep;

    rep = xcb_get_extension_data(context->HostX.conn, extension);

    return rep && rep->present;
}

static void
 hostx_set_fullscreen_hint(XephyrContext* context);

#define host_depth_matches_server(_vars) (context->HostX.depth == (_vars)->server_depth)

int
hostx_want_screen_geometry(KdScreenInfo *screen, int *width, int *height, int *x, int *y, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;

    if (scrpriv && (scrpriv->win_pre_existing != None ||
                    scrpriv->output != NULL ||
                    context->HostX.use_fullscreen == TRUE)) {
        *x = scrpriv->win_x;
        *y = scrpriv->win_y;
        *width = scrpriv->win_width;
        *height = scrpriv->win_height;
        return 1;
    }

    return 0;
}

void
hostx_add_screen(KdScreenInfo *screen, unsigned long win_id, int screen_num, Bool use_geometry, const char *output, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;
    int index = context->HostX.n_screens;

    context->HostX.n_screens += 1;
    context->HostX.screens = reallocarray(context->HostX.screens,
                                 context->HostX.n_screens, sizeof(context->HostX.screens[0]));
    context->HostX.screens[index] = screen;

    scrpriv->screen = screen;
    scrpriv->win_pre_existing = win_id;
    scrpriv->win_explicit_position = use_geometry;
    scrpriv->output = output;
}

void
hostx_set_display_name(char *name, XephyrContext* context)
{
    context->HostX.server_dpy_name = strdup(name);
}

void
hostx_set_screen_number(KdScreenInfo *screen, int number, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;

    if (scrpriv) {
        scrpriv->mynum = number;
        hostx_set_win_title(screen, "", context);
    }
}

void
hostx_set_win_title(KdScreenInfo *screen, const char *extra_text, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;

    if (!scrpriv)
        return;

    if (context->ephyrTitle) {
        xcb_icccm_set_wm_name(context->HostX.conn,
                              scrpriv->win,
                              XCB_ATOM_STRING,
                              8,
                              strlen(context->ephyrTitle),
                              context->ephyrTitle);
    } else {
#define BUF_LEN 256
        char buf[BUF_LEN + 1];

        memset(buf, 0, BUF_LEN + 1);
        snprintf(buf, BUF_LEN, "Xephyr on %s.%d %s",
                 context->HostX.server_dpy_name ? context->HostX.server_dpy_name : ":0",
                 scrpriv->mynum, (extra_text != NULL) ? extra_text : "");

        xcb_icccm_set_wm_name(context->HostX.conn,
                              scrpriv->win,
                              XCB_ATOM_STRING,
                              8,
                              strlen(buf),
                              buf);
        xcb_flush(context->HostX.conn);
    }
}

int
hostx_want_host_cursor(XephyrContext* context)
{
    return !context->HostX.use_sw_cursor;
}

void
hostx_use_sw_cursor(XephyrContext* context)
{
    context->HostX.use_sw_cursor = TRUE;
}

xcb_cursor_t
hostx_get_empty_cursor(XephyrContext* context)
{
    return context->HostX.empty_cursor;
}

int
hostx_want_preexisting_window(KdScreenInfo *screen)
{
    EphyrScrPriv *scrpriv = screen->driver;

    if (scrpriv && scrpriv->win_pre_existing) {
        return 1;
    }
    else {
        return 0;
    }
}

void
hostx_get_output_geometry(const char *output,
                          int *x, int *y,
                          int *width, int *height, XephyrContext* context)
{
    int i, name_len = 0, output_found = FALSE;
    char *name = NULL;
    xcb_generic_error_t *error;
    xcb_randr_query_version_cookie_t version_c;
    xcb_randr_query_version_reply_t *version_r;
    xcb_randr_get_screen_resources_cookie_t screen_resources_c;
    xcb_randr_get_screen_resources_reply_t *screen_resources_r;
    xcb_randr_output_t *randr_outputs;
    xcb_randr_get_output_info_cookie_t output_info_c;
    xcb_randr_get_output_info_reply_t *output_info_r;
    xcb_randr_get_crtc_info_cookie_t crtc_info_c;
    xcb_randr_get_crtc_info_reply_t *crtc_info_r;

    /* First of all, check for extension */
    if (!hostx_has_extension(&xcb_randr_id, context))
    {
        fprintf(stderr, "\nHost X server does not support RANDR extension (or it's disabled).\n");
        exit(1);
    }

    /* Check RandR version */
    version_c = xcb_randr_query_version(context->HostX.conn, 1, 2);
    version_r = xcb_randr_query_version_reply(context->HostX.conn,
                                              version_c,
                                              &error);

    if (error != NULL || version_r == NULL)
    {
        fprintf(stderr, "\nFailed to get RandR version supported by host X server.\n");
        exit(1);
    }
    else if (version_r->major_version < 1 || version_r->minor_version < 2)
    {
        free(version_r);
        fprintf(stderr, "\nHost X server doesn't support RandR 1.2, needed for -output usage.\n");
        exit(1);
    }

    free(version_r);

    /* Get list of outputs from screen resources */
    screen_resources_c = xcb_randr_get_screen_resources(context->HostX.conn,
                                                        context->HostX.winroot);
    screen_resources_r = xcb_randr_get_screen_resources_reply(context->HostX.conn,
                                                              screen_resources_c,
                                                              NULL);
    randr_outputs = xcb_randr_get_screen_resources_outputs(screen_resources_r);

    for (i = 0; !output_found && i < screen_resources_r->num_outputs; i++)
    {
        /* Get info on the output */
        output_info_c = xcb_randr_get_output_info(context->HostX.conn,
                                                  randr_outputs[i],
                                                  XCB_CURRENT_TIME);
        output_info_r = xcb_randr_get_output_info_reply(context->HostX.conn,
                                                        output_info_c,
                                                        NULL);

        /* Get output name */
        name_len = xcb_randr_get_output_info_name_length(output_info_r);
        name = malloc(name_len + 1);
        strncpy(name, (char*)xcb_randr_get_output_info_name(output_info_r), name_len);
        name[name_len] = '\0';

        if (!strcmp(name, output))
        {
            output_found = TRUE;

            /* Check if output is connected */
            if (output_info_r->crtc == XCB_NONE)
            {
                free(name);
                free(output_info_r);
                free(screen_resources_r);
                fprintf(stderr, "\nOutput %s is currently disabled (or not connected).\n", output);
                exit(1);
            }

            /* Get CRTC from output info */
            crtc_info_c = xcb_randr_get_crtc_info(context->HostX.conn,
                                                  output_info_r->crtc,
                                                  XCB_CURRENT_TIME);
            crtc_info_r = xcb_randr_get_crtc_info_reply(context->HostX.conn,
                                                        crtc_info_c,
                                                        NULL);

            /* Get CRTC geometry */
            *x = crtc_info_r->x;
            *y = crtc_info_r->y;
            *width = crtc_info_r->width;
            *height = crtc_info_r->height;

            free(crtc_info_r);
        }

        free(name);
        free(output_info_r);
    }

    free(screen_resources_r);

    if (!output_found)
    {
        fprintf(stderr, "\nOutput %s not available in host X server.\n", output);
        exit(1);
    }
}

void
hostx_use_fullscreen(XephyrContext* context)
{
    context->HostX.use_fullscreen = TRUE;
}

int
hostx_want_fullscreen(XephyrContext* context)
{
    return context->HostX.use_fullscreen;
}

static xcb_intern_atom_cookie_t cookie_WINDOW_STATE,
				cookie_WINDOW_STATE_FULLSCREEN;

static void
hostx_set_fullscreen_hint(XephyrContext* context)
{
    xcb_atom_t atom_WINDOW_STATE, atom_WINDOW_STATE_FULLSCREEN;
    int index;
    xcb_intern_atom_reply_t *reply;

    reply = xcb_intern_atom_reply(context->HostX.conn, cookie_WINDOW_STATE, NULL);
    atom_WINDOW_STATE = reply->atom;
    free(reply);

    reply = xcb_intern_atom_reply(context->HostX.conn, cookie_WINDOW_STATE_FULLSCREEN,
                                  NULL);
    atom_WINDOW_STATE_FULLSCREEN = reply->atom;
    free(reply);

    for (index = 0; index < context->HostX.n_screens; index++) {
        EphyrScrPriv *scrpriv = ((KdScreenInfo*)context->HostX.screens[index])->driver;
        xcb_change_property(context->HostX.conn,
                            PropModeReplace,
                            scrpriv->win,
                            atom_WINDOW_STATE,
                            XCB_ATOM_ATOM,
                            32,
                            1,
                            &atom_WINDOW_STATE_FULLSCREEN);
    }
}

static void
hostx_toggle_damage_debug(XephyrContext* context)
{
    context->HostXWantDamageDebug ^= 1;
}

void
hostx_handle_signal(int signum, XephyrContext* context)
{
    hostx_toggle_damage_debug(context);
    EPHYR_DBG("Signal caught. Damage Debug:%i\n", context->HostXWantDamageDebug);
}

void
hostx_use_resname(char *name, int fromcmd, XephyrContext* context)
{
    context->ephyrResName = name;
    context->ephyrResNameFromCmd = fromcmd;
}

void
hostx_set_title(char *title, XephyrContext* context)
{
    context->ephyrTitle = title;
}

#ifdef __SUNPRO_C
/* prevent "Function has no return statement" error for x_io_error_handler */
#pragma does_not_return(exit)
#endif

static void
hostx_init_shm(XephyrContext* context)
{
    /* Try to get share memory ximages for a little bit more speed */
    if (!hostx_has_extension(&xcb_shm_id, context) || getenv("XEPHYR_NO_SHM")) {
        context->HostX.have_shm = FALSE;
    } else {
        xcb_generic_error_t *error = NULL;
        xcb_shm_query_version_cookie_t cookie;
        xcb_shm_query_version_reply_t *reply;

        context->HostX.have_shm = TRUE;
        context->HostX.have_shm_fd_passing = FALSE;
        cookie = xcb_shm_query_version(context->HostX.conn);
        reply = xcb_shm_query_version_reply(context->HostX.conn, cookie, &error);
        if (reply) {
            context->HostX.have_shm_fd_passing =
                    (reply->major_version == 1 && reply->minor_version >= 2) ||
                    reply->major_version > 1;
            free(reply);
        }
        free(error);
    }
}

static Bool
hostx_create_shm_segment(xcb_shm_segment_info_t *shminfo, size_t size, XephyrContext* context)
{
    shminfo->shmaddr = NULL;

    if (context->HostX.have_shm_fd_passing) {
        xcb_generic_error_t *error = NULL;
        xcb_shm_create_segment_cookie_t cookie;
        xcb_shm_create_segment_reply_t *reply;

        shminfo->shmseg = xcb_generate_id(context->HostX.conn);
        cookie = xcb_shm_create_segment(context->HostX.conn, shminfo->shmseg, size, TRUE);
        reply = xcb_shm_create_segment_reply(context->HostX.conn, cookie, &error);
        if (reply) {
            int *fds = reply->nfd == 1 ?
                        xcb_shm_create_segment_reply_fds(context->HostX.conn, reply) : NULL;
            if (fds) {
                shminfo->shmaddr =
                        (uint8_t *)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fds[0], 0);
                close(fds[0]);
                if (shminfo->shmaddr == MAP_FAILED)
                    shminfo->shmaddr = NULL;
            }
            if (!shminfo->shmaddr)
                xcb_shm_detach(context->HostX.conn, shminfo->shmseg);

            free(reply);
        }
        free(error);
    } else {
        shminfo->shmid = shmget(IPC_PRIVATE, size, IPC_CREAT|0666);
        if (shminfo->shmid != -1) {
            shminfo->shmaddr = shmat(shminfo->shmid, 0, 0);
            if (shminfo->shmaddr == (void *)-1) {
                shminfo->shmaddr = NULL;
            } else {
                xcb_generic_error_t *error = NULL;
                xcb_void_cookie_t cookie;

                shmctl(shminfo->shmid, IPC_RMID, 0);

                shminfo->shmseg = xcb_generate_id(context->HostX.conn);
                cookie = xcb_shm_attach_checked(context->HostX.conn, shminfo->shmseg, shminfo->shmid, TRUE);
                error = xcb_request_check(context->HostX.conn, cookie);

                if (error) {
                    free(error);
                    shmdt(shminfo->shmaddr);
                    shminfo->shmaddr = NULL;
                }
            }
        }
    }

    return shminfo->shmaddr != NULL;
}

static void
hostx_destroy_shm_segment(xcb_shm_segment_info_t *shminfo, size_t size, XephyrContext* context)
{
    xcb_shm_detach(context->HostX.conn, shminfo->shmseg);

    if (context->HostX.have_shm_fd_passing)
        munmap(shminfo->shmaddr, size);
    else
        shmdt(shminfo->shmaddr);

    shminfo->shmaddr = NULL;
}

int
hostx_init(XephyrContext* context)
{
    uint32_t attrs[2];
    uint32_t attr_mask = 0;
    xcb_pixmap_t cursor_pxm;
    xcb_gcontext_t cursor_gc;
    uint16_t red, green, blue;
    uint32_t pixel;
    int index;
    char *tmpstr;
    char *class_hint;
    size_t class_len;
    xcb_screen_t *xscreen;
    xcb_rectangle_t rect = { 0, 0, 1, 1 };

    attrs[0] =
        XCB_EVENT_MASK_BUTTON_PRESS
        | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_POINTER_MOTION
        | XCB_EVENT_MASK_KEY_PRESS
        | XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    attr_mask |= XCB_CW_EVENT_MASK;

    EPHYR_DBG("mark");
#ifdef GLAMOR
    if (context->ephyr_glamor)
        context->HostX.conn = ephyr_glamor_connect();
    else
#endif
        context->HostX.conn = xcb_connect(NULL, &context->HostX.screen);
    if (!context->HostX.conn || xcb_connection_has_error(context->HostX.conn)) {
        fprintf(stderr, "\nXephyr cannot open host context->display. Is DISPLAY set?\n");
        exit(1);
    }

    xscreen = xcb_aux_get_screen(context->HostX.conn, context->HostX.screen);
    context->HostX.winroot = xscreen->root;
    context->HostX.gc = xcb_generate_id(context->HostX.conn);
    context->HostX.depth = xscreen->root_depth;
#ifdef GLAMOR
    if (context->ephyr_glamor) {
        context->HostX.visual = ephyr_glamor_get_visual(context);
        if (context->HostX.visual->visual_id != xscreen->root_visual) {
            attrs[1] = xcb_generate_id(context->HostX.conn);
            attr_mask |= XCB_CW_COLORMAP;
            xcb_create_colormap(context->HostX.conn,
                                XCB_COLORMAP_ALLOC_NONE,
                                attrs[1],
                                context->HostX.winroot,
                                context->HostX.visual->visual_id);
        }
    } else
#endif
        context->HostX.visual = xcb_aux_find_visual_by_id(xscreen,xscreen->root_visual);

    xcb_create_gc(context->HostX.conn, context->HostX.gc, context->HostX.winroot, 0, NULL);
    cookie_WINDOW_STATE = xcb_intern_atom(context->HostX.conn, FALSE,
                                          strlen("_NET_WM_STATE"),
                                          "_NET_WM_STATE");
    cookie_WINDOW_STATE_FULLSCREEN =
        xcb_intern_atom(context->HostX.conn, FALSE,
                        strlen("_NET_WM_STATE_FULLSCREEN"),
                        "_NET_WM_STATE_FULLSCREEN");

    for (index = 0; index < context->HostX.n_screens; index++) {
        KdScreenInfo *screen = context->HostX.screens[index];
        EphyrScrPriv *scrpriv = screen->driver;

        scrpriv->win = xcb_generate_id(context->HostX.conn);
        scrpriv->server_depth = context->HostX.depth;
        scrpriv->ximg = NULL;
        scrpriv->win_x = 0;
        scrpriv->win_y = 0;

        if (scrpriv->win_pre_existing != XCB_WINDOW_NONE) {
            xcb_get_geometry_reply_t *prewin_geom;
            xcb_get_geometry_cookie_t cookie;
            xcb_generic_error_t *e = NULL;

            /* Get screen size from existing window */
            cookie = xcb_get_geometry(context->HostX.conn,
                                      scrpriv->win_pre_existing);
            prewin_geom = xcb_get_geometry_reply(context->HostX.conn, cookie, &e);

            if (e) {
                free(e);
                free(prewin_geom);
                fprintf (stderr, "\nXephyr -parent window' does not exist!\n");
                exit (1);
            }

            scrpriv->win_width  = prewin_geom->width;
            scrpriv->win_height = prewin_geom->height;

            free(prewin_geom);

            xcb_create_window(context->HostX.conn,
                              XCB_COPY_FROM_PARENT,
                              scrpriv->win,
                              scrpriv->win_pre_existing,
                              0,0,
                              scrpriv->win_width,
                              scrpriv->win_height,
                              0,
                              XCB_WINDOW_CLASS_COPY_FROM_PARENT,
                              context->HostX.visual->visual_id,
                              attr_mask,
                              attrs);
        }
        else {
            xcb_create_window(context->HostX.conn,
                              XCB_COPY_FROM_PARENT,
                              scrpriv->win,
                              context->HostX.winroot,
                              0,0,100,100, /* will resize */
                              0,
                              XCB_WINDOW_CLASS_COPY_FROM_PARENT,
                              context->HostX.visual->visual_id,
                              attr_mask,
                              attrs);

            hostx_set_win_title(screen,
                                "(ctrl+shift grabs mouse and keyboard)", context);

            if (context->HostX.use_fullscreen) {
                scrpriv->win_width  = xscreen->width_in_pixels;
                scrpriv->win_height = xscreen->height_in_pixels;

                hostx_set_fullscreen_hint(context);
            }
            else if (scrpriv->output) {
                hostx_get_output_geometry(scrpriv->output,
                                          &scrpriv->win_x,
                                          &scrpriv->win_y,
                                          &scrpriv->win_width,
                                          &scrpriv->win_height, context);

                context->HostX.use_fullscreen = TRUE;
                hostx_set_fullscreen_hint(context);
            }


            tmpstr = getenv("RESOURCE_NAME");
            if (tmpstr && (!context->ephyrResNameFromCmd))
                context->ephyrResName = tmpstr;
            if (!context->ephyrResName)
                context->ephyrResName = "Xephyr";
            class_len = strlen(context->ephyrResName) + 1 + strlen("Xephyr") + 1;
            class_hint = malloc(class_len);
            if (class_hint) {
                strcpy(class_hint, context->ephyrResName);
                strcpy(class_hint + strlen(context->ephyrResName) + 1, "Xephyr");
                xcb_change_property(context->HostX.conn,
                                    XCB_PROP_MODE_REPLACE,
                                    scrpriv->win,
                                    XCB_ATOM_WM_CLASS,
                                    XCB_ATOM_STRING,
                                    8,
                                    class_len,
                                    class_hint);
                free(class_hint);
            }
        }
    }

    if (!xcb_aux_parse_color((char*)"red", &red, &green, &blue)) {
        xcb_lookup_color_cookie_t c =
            xcb_lookup_color(context->HostX.conn, xscreen->default_colormap, 3, "red");
        xcb_lookup_color_reply_t *reply =
            xcb_lookup_color_reply(context->HostX.conn, c, NULL);
        red = reply->exact_red;
        green = reply->exact_green;
        blue = reply->exact_blue;
        free(reply);
    }

    {
        xcb_alloc_color_cookie_t c = xcb_alloc_color(context->HostX.conn,
                                                     xscreen->default_colormap,
                                                     red, green, blue);
        xcb_alloc_color_reply_t *r = xcb_alloc_color_reply(context->HostX.conn, c, NULL);
        red = r->red;
        green = r->green;
        blue = r->blue;
        pixel = r->pixel;
        free(r);
    }

    xcb_change_gc(context->HostX.conn, context->HostX.gc, XCB_GC_FOREGROUND, &pixel);

    cursor_pxm = xcb_generate_id(context->HostX.conn);
    xcb_create_pixmap(context->HostX.conn, 1, cursor_pxm, context->HostX.winroot, 1, 1);
    cursor_gc = xcb_generate_id(context->HostX.conn);
    pixel = 0;
    xcb_create_gc(context->HostX.conn, cursor_gc, cursor_pxm,
                  XCB_GC_FOREGROUND, &pixel);
    xcb_poly_fill_rectangle(context->HostX.conn, cursor_pxm, cursor_gc, 1, &rect);
    xcb_free_gc(context->HostX.conn, cursor_gc);
    context->HostX.empty_cursor = xcb_generate_id(context->HostX.conn);
    xcb_create_cursor(context->HostX.conn,
                      context->HostX.empty_cursor,
                      cursor_pxm, cursor_pxm,
                      0,0,0,
                      0,0,0,
                      1,1);
    xcb_free_pixmap(context->HostX.conn, cursor_pxm);
    if (!hostx_want_host_cursor(context)) {
        context->CursorVisible = TRUE;
        /* Ditch the cursor, we provide our 'own' */
        for (index = 0; index < context->HostX.n_screens; index++) {
            KdScreenInfo *screen = context->HostX.screens[index];
            EphyrScrPriv *scrpriv = screen->driver;

            xcb_change_window_attributes(context->HostX.conn,
                                         scrpriv->win,
                                         XCB_CW_CURSOR,
                                         &context->HostX.empty_cursor);
        }
    }

    hostx_init_shm(context);
    if (context->HostX.have_shm) {
        /* Really really check we have shm - better way ?*/
        xcb_shm_segment_info_t shminfo;
        if (!hostx_create_shm_segment(&shminfo, 1, context)) {
            fprintf(stderr, "\nXephyr unable to use SHM XImages\n");
            context->HostX.have_shm = FALSE;
        } else {
            hostx_destroy_shm_segment(&shminfo, 1, context);
        }
    } else {
        fprintf(stderr, "\nXephyr unable to use SHM XImages\n");
    }

    xcb_flush(context->HostX.conn);

    /* Setup the pause time between paints when debugging updates */

    context->HostX.damage_debug_msec = 20000;    /* 1/50 th of a second */

    if (getenv("XEPHYR_PAUSE")) {
        context->HostX.damage_debug_msec = strtol(getenv("XEPHYR_PAUSE"), NULL, 0);
        EPHYR_DBG("pause is %li\n", context->HostX.damage_debug_msec);
    }

    return 1;
}

int
hostx_get_depth(XephyrContext* context)
{
    return context->HostX.depth;
}

int
hostx_get_server_depth(KdScreenInfo *screen, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;

    return scrpriv ? scrpriv->server_depth : 0;
}

int
hostx_get_bpp(KdScreenInfo *screen, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;

    if (!scrpriv)
        return 0;

    if (host_depth_matches_server(scrpriv))
        return context->HostX.visual->bits_per_rgb_value;
    else
        return scrpriv->server_depth; /*XXX correct ?*/
}

void
hostx_get_visual_masks(KdScreenInfo *screen,
                       CARD32 *rmsk, CARD32 *gmsk, CARD32 *bmsk, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;

    if (!scrpriv)
        return;

    if (host_depth_matches_server(scrpriv)) {
        *rmsk = context->HostX.visual->red_mask;
        *gmsk = context->HostX.visual->green_mask;
        *bmsk = context->HostX.visual->blue_mask;
    }
    else if (scrpriv->server_depth == 16) {
        /* Assume 16bpp 565 */
        *rmsk = 0xf800;
        *gmsk = 0x07e0;
        *bmsk = 0x001f;
    }
    else {
        *rmsk = 0x0;
        *gmsk = 0x0;
        *bmsk = 0x0;
    }
}

static int
hostx_calculate_color_shift(unsigned long mask)
{
    int shift = 1;

    /* count # of bits in mask */
    while ((mask = (mask >> 1)))
        shift++;
    /* cmap entry is an unsigned char so adjust it by size of that */
    shift = shift - sizeof(unsigned char) * 8;
    if (shift < 0)
        shift = 0;
    return shift;
}

void
hostx_set_cmap_entry(ScreenPtr pScreen, unsigned char idx,
                     unsigned char r, unsigned char g, unsigned char b, XephyrContext* context)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
/* need to calculate the shifts for RGB because server could be BGR. */
/* XXX Not sure if this is correct for 8 on 16, but this works for 8 on 24.*/
    static int rshift, bshift, gshift = 0;
    static int first_time = 1;

    if (first_time) {
        first_time = 0;
        rshift = hostx_calculate_color_shift(context->HostX.visual->red_mask);
        gshift = hostx_calculate_color_shift(context->HostX.visual->green_mask);
        bshift = hostx_calculate_color_shift(context->HostX.visual->blue_mask);
    }
    scrpriv->cmap[idx] = ((r << rshift) & context->HostX.visual->red_mask) |
        ((g << gshift) & context->HostX.visual->green_mask) |
        ((b << bshift) & context->HostX.visual->blue_mask);
}

/**
 * hostx_screen_init creates the XImage that will contain the front buffer of
 * the ephyr screen, and possibly offscreen memory.
 *
 * @param width width of the screen
 * @param height height of the screen
 * @param buffer_height  height of the rectangle to be allocated.
 *
 * hostx_screen_init() creates an XImage, using MIT-SHM if it's available.
 * buffer_height can be used to create a larger offscreen buffer, which is used
 * by fakexa for storing offscreen pixmap data.
 */
void *
hostx_screen_init(KdScreenInfo *screen,
                  int x, int y,
                  int width, int height, int buffer_height,
                  int *bytes_per_line, int *bits_per_pixel, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;
    Bool shm_success = FALSE;

    if (!scrpriv) {
        fprintf(stderr, "%s: Error in accessing hostx data\n", __func__);
        exit(1);
    }

    EPHYR_DBG("host_screen=%p x=%d, y=%d, wxh=%dx%d, buffer_height=%d",
              screen, x, y, width, height, buffer_height);

    if (scrpriv->ximg != NULL) {
        /* Free up the image data if previously used
         * i.ie called by server reset
         */

        if (context->HostX.have_shm) {
            xcb_image_destroy(scrpriv->ximg);
            hostx_destroy_shm_segment(&scrpriv->shminfo, scrpriv->shmsize, context);
        }
        else {
            free(scrpriv->ximg->data);
            scrpriv->ximg->data = NULL;

            xcb_image_destroy(scrpriv->ximg);
        }
    }

    if (!context->ephyr_glamor && context->HostX.have_shm) {
        scrpriv->ximg = xcb_image_create_native(context->HostX.conn,
                                                width,
                                                buffer_height,
                                                XCB_IMAGE_FORMAT_Z_PIXMAP,
                                                context->HostX.depth,
                                                NULL,
                                                ~0,
                                                NULL);

        scrpriv->shmsize = scrpriv->ximg->stride * buffer_height;
        if (!hostx_create_shm_segment(&scrpriv->shminfo,
                                      scrpriv->shmsize, context)) {
            EPHYR_DBG
                ("Can't create SHM Segment, falling back to plain XImages");
            context->HostX.have_shm = FALSE;
            xcb_image_destroy(scrpriv->ximg);
        }
        else {
            EPHYR_DBG("SHM segment created %p", scrpriv->shminfo.shmaddr);
            scrpriv->ximg->data = scrpriv->shminfo.shmaddr;
            shm_success = TRUE;
        }
    }

    if (!context->ephyr_glamor && !shm_success) {
        EPHYR_DBG("Creating image %dx%d for screen scrpriv=%p\n",
                  width, buffer_height, scrpriv);
        scrpriv->ximg = xcb_image_create_native(context->HostX.conn,
                                                    width,
                                                    buffer_height,
                                                    XCB_IMAGE_FORMAT_Z_PIXMAP,
                                                    context->HostX.depth,
                                                    NULL,
                                                    ~0,
                                                    NULL);

        /* Match server byte order so that the image can be converted to
         * the native byte order by xcb_image_put() before drawing */
        if (host_depth_matches_server(scrpriv))
            scrpriv->ximg->byte_order = IMAGE_BYTE_ORDER;

        scrpriv->ximg->data =
            xallocarray(scrpriv->ximg->stride, buffer_height);
    }

    if (!context->HostX.size_set_from_configure)
    {
        uint32_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        uint32_t values[2] = {width, height};
        xcb_configure_window(context->HostX.conn, scrpriv->win, mask, values);
    }

    if (scrpriv->win_pre_existing == None && !context->EphyrWantResize) {
        /* Ask the WM to keep our size static */
        xcb_size_hints_t size_hints = {0};
        size_hints.max_width = size_hints.min_width = width;
        size_hints.max_height = size_hints.min_height = height;
        size_hints.flags = (XCB_ICCCM_SIZE_HINT_P_MIN_SIZE |
                            XCB_ICCCM_SIZE_HINT_P_MAX_SIZE);
        xcb_icccm_set_wm_normal_hints(context->HostX.conn, scrpriv->win,
                                      &size_hints);
    }

#ifdef GLAMOR
    if (!context->ephyr_glamor_skip_present)
#endif
        xcb_map_window(context->HostX.conn, scrpriv->win);

    /* Set explicit window position if it was informed in
     * -screen option (WxH+X or WxH+X+Y). Otherwise, accept the
     * position set by WM.
     * The trick here is putting this code after xcb_map_window() call,
     * so these values won't be overridden by WM. */
    if (scrpriv->win_explicit_position)
    {
        uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
        uint32_t values[2] = {x, y};
        xcb_configure_window(context->HostX.conn, scrpriv->win, mask, values);
    }


    xcb_aux_sync(context->HostX.conn);

    scrpriv->win_width = width;
    scrpriv->win_height = height;
    scrpriv->win_x = x;
    scrpriv->win_y = y;

#ifdef GLAMOR
    if (context->ephyr_glamor) {
        *bytes_per_line = 0;
        ephyr_glamor_set_window_size(scrpriv->glamor,
                                     scrpriv->win_width, scrpriv->win_height);
        return NULL;
    } else
#endif
    if (host_depth_matches_server(scrpriv)) {
        *bytes_per_line = scrpriv->ximg->stride;
        *bits_per_pixel = scrpriv->ximg->bpp;

        EPHYR_DBG("Host matches server");
        return scrpriv->ximg->data;
    }
    else {
        int bytes_per_pixel = scrpriv->server_depth >> 3;
        int stride = (width * bytes_per_pixel + 0x3) & ~0x3;

        *bytes_per_line = stride;
        *bits_per_pixel = scrpriv->server_depth;

        EPHYR_DBG("server bpp %i", bytes_per_pixel);
        scrpriv->fb_data = xallocarray (stride, buffer_height);
        return scrpriv->fb_data;
    }
}

static void hostx_paint_debug_rect(KdScreenInfo *screen,
                                   int x, int y, int width, int height, XephyrContext* context);

void
hostx_paint_rect(KdScreenInfo *screen,
                 int sx, int sy, int dx, int dy, int width, int height, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;

    EPHYR_DBG("painting in screen %d\n", scrpriv->mynum);

#ifdef GLAMOR
    if (context->ephyr_glamor) {
        BoxRec box;
        RegionRec region;

        box.x1 = dx;
        box.y1 = dy;
        box.x2 = dx + width;
        box.y2 = dy + height;

        RegionInit(&region, &box, 1, context);
        ephyr_glamor_damage_redisplay(scrpriv->glamor, &region, context);
        RegionUninit(&region);
        return;
    }
#endif

    /*
     *  Copy the image data updated by the shadow layer
     *  on to the window
     */

    if (context->HostXWantDamageDebug) {
        hostx_paint_debug_rect(screen, dx, dy, width, height, context);
    }

    /*
     * If the depth of the ephyr server is less than that of the host,
     * the kdrive fb does not point to the ximage data but to a buffer
     * ( fb_data ), we shift the various bits from this onto the XImage
     * so they match the host.
     *
     * Note, This code is pretty new ( and simple ) so may break on
     *       endian issues, 32 bpp host etc.
     *       Not sure if 8bpp case is right either.
     *       ... and it will be slower than the matching depth case.
     */

    if (!host_depth_matches_server(scrpriv)) {
        int x, y, idx, bytes_per_pixel = (scrpriv->server_depth >> 3);
        int stride = (scrpriv->win_width * bytes_per_pixel + 0x3) & ~0x3;
        unsigned char r, g, b;
        unsigned long host_pixel;

        EPHYR_DBG("Unmatched host depth scrpriv=%p\n", scrpriv);
        for (y = sy; y < sy + height; y++)
            for (x = sx; x < sx + width; x++) {
                idx = y * stride + x * bytes_per_pixel;

                switch (scrpriv->server_depth) {
                case 16:
                {
                    unsigned short pixel =
                        *(unsigned short *) (scrpriv->fb_data + idx);

                    r = ((pixel & 0xf800) >> 8);
                    g = ((pixel & 0x07e0) >> 3);
                    b = ((pixel & 0x001f) << 3);

                    host_pixel = (r << 16) | (g << 8) | (b);

                    xcb_image_put_pixel(scrpriv->ximg, x, y, host_pixel);
                    break;
                }
                case 8:
                {
                    unsigned char pixel =
                        *(unsigned char *) (scrpriv->fb_data + idx);
                    xcb_image_put_pixel(scrpriv->ximg, x, y,
                                        scrpriv->cmap[pixel]);
                    break;
                }
                default:
                    break;
                }
            }
    }

    if (context->HostX.have_shm) {
        xcb_image_shm_put(context->HostX.conn, scrpriv->win,
                          context->HostX.gc, scrpriv->ximg,
                          scrpriv->shminfo,
                          sx, sy, dx, dy, width, height, FALSE);
    }
    else {
        xcb_image_t *subimg = xcb_image_subimage(scrpriv->ximg, sx, sy,
                                                 width, height, 0, 0, 0);
        xcb_image_t *img = xcb_image_native(context->HostX.conn, subimg, 1);
        xcb_image_put(context->HostX.conn, scrpriv->win, context->HostX.gc, img, dx, dy, 0);
        if (subimg != img)
            xcb_image_destroy(img);
        xcb_image_destroy(subimg);
    }

    xcb_aux_sync(context->HostX.conn);
}

static void
hostx_paint_debug_rect(KdScreenInfo *screen,
                       int x, int y, int width, int height, XephyrContext* context)
{
    EphyrScrPriv *scrpriv = screen->driver;
    struct timespec tspec;
    xcb_rectangle_t rect = { .x = x, .y = y, .width = width, .height = height };
    xcb_void_cookie_t cookie;
    xcb_generic_error_t *e;

    tspec.tv_sec = context->HostX.damage_debug_msec / (1000000);
    tspec.tv_nsec = (context->HostX.damage_debug_msec % 1000000) * 1000;

    EPHYR_DBG("msec: %li tv_sec %li, tv_msec %li",
              context->HostX.damage_debug_msec, tspec.tv_sec, tspec.tv_nsec);

    /* fprintf(stderr, "Xephyr updating: %i+%i %ix%i\n", x, y, width, height); */

    cookie = xcb_poly_fill_rectangle_checked(context->HostX.conn, scrpriv->win,
                                             context->HostX.gc, 1, &rect);
    e = xcb_request_check(context->HostX.conn, cookie);
    free(e);

    /* nanosleep seems to work better than usleep for me... */
    nanosleep(&tspec, NULL);
}

Bool
hostx_load_keymap(KeySymsPtr keySyms, CARD8 *modmap, XkbControlsPtr controls, XephyrContext* context)
{
    int min_keycode, max_keycode;
    int map_width;
    size_t i, j;
    int keymap_len;
    xcb_keysym_t *keymap;
    xcb_keycode_t *modifier_map;
    xcb_get_keyboard_mapping_cookie_t mapping_c;
    xcb_get_keyboard_mapping_reply_t *mapping_r;
    xcb_get_modifier_mapping_cookie_t modifier_c;
    xcb_get_modifier_mapping_reply_t *modifier_r;
    xcb_xkb_use_extension_cookie_t use_c;
    xcb_xkb_use_extension_reply_t *use_r;
    xcb_xkb_get_controls_cookie_t controls_c;
    xcb_xkb_get_controls_reply_t *controls_r;

    /* First of all, collect host X server's
     * min_keycode and max_keycode, which are
     * independent from XKB support. */
    min_keycode = xcb_get_setup(context->HostX.conn)->min_keycode;
    max_keycode = xcb_get_setup(context->HostX.conn)->max_keycode;

    EPHYR_DBG("min: %d, max: %d", min_keycode, max_keycode);

    keySyms->minKeyCode = min_keycode;
    keySyms->maxKeyCode = max_keycode;

    /* Check for XKB availability in host X server */
    if (!hostx_has_extension(&xcb_xkb_id, context)) {
        EPHYR_LOG_ERROR("XKB extension is not supported in host X server.");
        return FALSE;
    }

    use_c = xcb_xkb_use_extension(context->HostX.conn,
                                  XCB_XKB_MAJOR_VERSION,
                                  XCB_XKB_MINOR_VERSION);
    use_r = xcb_xkb_use_extension_reply(context->HostX.conn, use_c, NULL);

    if (!use_r) {
        EPHYR_LOG_ERROR("Couldn't use XKB extension.");
        return FALSE;
    } else if (!use_r->supported) {
        EPHYR_LOG_ERROR("XKB extension is not supported in host X server.");
        free(use_r);
        return FALSE;
    }

    free(use_r);

    /* Send all needed XCB requests at once,
     * and process the replies as needed. */
    mapping_c = xcb_get_keyboard_mapping(context->HostX.conn,
                                         min_keycode,
                                         max_keycode - min_keycode + 1);
    modifier_c = xcb_get_modifier_mapping(context->HostX.conn);
    controls_c = xcb_xkb_get_controls(context->HostX.conn,
                                      XCB_XKB_ID_USE_CORE_KBD);

    mapping_r = xcb_get_keyboard_mapping_reply(context->HostX.conn,
                                               mapping_c,
                                               NULL);

    if (!mapping_r) {
        EPHYR_LOG_ERROR("xcb_get_keyboard_mapping_reply() failed.");
        return FALSE;
    }

    map_width = mapping_r->keysyms_per_keycode;
    keymap = xcb_get_keyboard_mapping_keysyms(mapping_r);
    keymap_len = xcb_get_keyboard_mapping_keysyms_length(mapping_r);

    keySyms->mapWidth = map_width;
    keySyms->map = calloc(keymap_len, sizeof(KeySym));

    if (!keySyms->map) {
        EPHYR_LOG_ERROR("Failed to allocate KeySym map.");
        free(mapping_r);
        return FALSE;
    }

    for (i = 0; i < keymap_len; i++) {
        keySyms->map[i] = keymap[i];
    }

    free(mapping_r);

    modifier_r = xcb_get_modifier_mapping_reply(context->HostX.conn,
                                                modifier_c,
                                                NULL);

    if (!modifier_r) {
        EPHYR_LOG_ERROR("xcb_get_modifier_mapping_reply() failed.");
        return FALSE;
    }

    modifier_map = xcb_get_modifier_mapping_keycodes(modifier_r);
    memset(modmap, 0, sizeof(CARD8) * MAP_LENGTH);

    for (j = 0; j < 8; j++) {
        for (i = 0; i < modifier_r->keycodes_per_modifier; i++) {
            CARD8 keycode;

            if ((keycode = modifier_map[j * modifier_r->keycodes_per_modifier + i])) {
                modmap[keycode] |= 1 << j;
            }
        }
    }

    free(modifier_r);

    controls_r = xcb_xkb_get_controls_reply(context->HostX.conn,
                                            controls_c,
                                            NULL);

    if (!controls_r) {
        EPHYR_LOG_ERROR("xcb_xkb_get_controls_reply() failed.");
        return FALSE;
    }

    controls->enabled_ctrls = controls_r->enabledControls;

    for (i = 0; i < XkbPerKeyBitArraySize; i++) {
        controls->per_key_repeat[i] = controls_r->perKeyRepeat[i];
    }

    free(controls_r);

    return TRUE;
}

void
hostx_size_set_from_configure(Bool ss, XephyrContext* context)
{
    context->HostX.size_set_from_configure = ss;
}

xcb_connection_t *
hostx_get_xcbconn(XephyrContext* context)
{
    return context->HostX.conn;
}

xcb_generic_event_t *
hostx_get_event(Bool queued_only, XephyrContext* context)
{
    xcb_generic_event_t *xev;

    if (context->HostX.saved_event) {
        xev = context->HostX.saved_event;
        context->HostX.saved_event = NULL;
    } else {
        if (queued_only)
            xev = xcb_poll_for_queued_event(context->HostX.conn);
        else
            xev = xcb_poll_for_event(context->HostX.conn);
    }
    return xev;
}

Bool
hostx_has_queued_event(XephyrContext* context)
{
    if (!context->HostX.saved_event)
        context->HostX.saved_event = xcb_poll_for_queued_event(context->HostX.conn);
    return context->HostX.saved_event != NULL;
}

int
hostx_get_screen(XephyrContext* context)
{
    return context->HostX.screen;
}

int
hostx_get_fd(XephyrContext* context)
{
    return xcb_get_file_descriptor(context->HostX.conn);
}

int
hostx_get_window(int a_screen_number, XephyrContext* context)
{
    EphyrScrPriv *scrpriv;
    if (a_screen_number < 0 || a_screen_number >= context->HostX.n_screens) {
        EPHYR_LOG_ERROR("bad screen number:%d\n", a_screen_number);
        return 0;
    }
    scrpriv = ((KdScreenInfo*)context->HostX.screens[a_screen_number])->driver;
    return scrpriv->win;
}

int
hostx_get_window_attributes(int a_window, EphyrHostWindowAttributes * a_attrs, XephyrContext* context)
{
    xcb_get_geometry_cookie_t geom_cookie;
    xcb_get_window_attributes_cookie_t attr_cookie;
    xcb_get_geometry_reply_t *geom_reply;
    xcb_get_window_attributes_reply_t *attr_reply;

    geom_cookie = xcb_get_geometry(context->HostX.conn, a_window);
    attr_cookie = xcb_get_window_attributes(context->HostX.conn, a_window);
    geom_reply = xcb_get_geometry_reply(context->HostX.conn, geom_cookie, NULL);
    attr_reply = xcb_get_window_attributes_reply(context->HostX.conn, attr_cookie, NULL);

    a_attrs->x = geom_reply->x;
    a_attrs->y = geom_reply->y;
    a_attrs->width = geom_reply->width;
    a_attrs->height = geom_reply->height;
    a_attrs->visualid = attr_reply->visual;

    free(geom_reply);
    free(attr_reply);
    return TRUE;
}

int
hostx_get_visuals_info(EphyrHostVisualInfo ** a_visuals, int *a_num_entries, XephyrContext* context)
{
    Bool is_ok = FALSE;
    EphyrHostVisualInfo *host_visuals = NULL;
    int nb_items = 0, i = 0, screen_num;
    xcb_screen_iterator_t screens;
    xcb_depth_iterator_t depths;

    EPHYR_RETURN_VAL_IF_FAIL(a_visuals && a_num_entries, FALSE);
    EPHYR_LOG("enter\n");

    screens = xcb_setup_roots_iterator(xcb_get_setup(context->HostX.conn));
    for (screen_num = 0; screens.rem; screen_num++, xcb_screen_next(&screens)) {
        depths = xcb_screen_allowed_depths_iterator(screens.data);
        for (; depths.rem; xcb_depth_next(&depths)) {
            xcb_visualtype_t *visuals = xcb_depth_visuals(depths.data);
            EphyrHostVisualInfo *tmp_visuals =
                reallocarray(host_visuals,
                             nb_items + depths.data->visuals_len,
                             sizeof(EphyrHostVisualInfo));
            if (!tmp_visuals) {
                goto out;
            }
            host_visuals = tmp_visuals;
            for (i = 0; i < depths.data->visuals_len; i++) {
                host_visuals[nb_items + i].visualid = visuals[i].visual_id;
                host_visuals[nb_items + i].screen = screen_num;
                host_visuals[nb_items + i].depth = depths.data->depth;
                host_visuals[nb_items + i].class = visuals[i]._class;
                host_visuals[nb_items + i].red_mask = visuals[i].red_mask;
                host_visuals[nb_items + i].green_mask = visuals[i].green_mask;
                host_visuals[nb_items + i].blue_mask = visuals[i].blue_mask;
                host_visuals[nb_items + i].colormap_size = visuals[i].colormap_entries;
                host_visuals[nb_items + i].bits_per_rgb = visuals[i].bits_per_rgb_value;
            }
            nb_items += depths.data->visuals_len;
        }
    }

    EPHYR_LOG("host advertises %d visuals\n", nb_items);
    *a_visuals = host_visuals;
    *a_num_entries = nb_items;
    host_visuals = NULL;

    is_ok = TRUE;
out:
    free(host_visuals);
    host_visuals = NULL;
    EPHYR_LOG("leave\n");
    return is_ok;

}

int
hostx_create_window(int a_screen_number,
                    EphyrBox * a_geometry,
                    int a_visual_id, int *a_host_peer /*out parameter */, XephyrContext* context)
{
    Bool is_ok = FALSE;
    xcb_window_t win;
    int winmask = 0;
    uint32_t attrs[2];
    xcb_screen_t *screen = xcb_aux_get_screen(context->HostX.conn, hostx_get_screen(context));
    xcb_visualtype_t *visual;
    int depth = 0;
    EphyrScrPriv *scrpriv = ((KdScreenInfo*)context->HostX.screens[a_screen_number])->driver;

    EPHYR_RETURN_VAL_IF_FAIL(screen && a_geometry, FALSE);

    EPHYR_LOG("enter\n");

    visual = xcb_aux_find_visual_by_id(screen, a_visual_id);
    if (!visual) {
        EPHYR_LOG_ERROR ("argh, could not find a remote visual with id:%d\n",
                         a_visual_id);
        goto out;
    }
    depth = xcb_aux_get_depth_of_visual(screen, a_visual_id);

    winmask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    attrs[0] = XCB_EVENT_MASK_BUTTON_PRESS
              |XCB_EVENT_MASK_BUTTON_RELEASE
              |XCB_EVENT_MASK_POINTER_MOTION
              |XCB_EVENT_MASK_KEY_PRESS
              |XCB_EVENT_MASK_KEY_RELEASE
              |XCB_EVENT_MASK_EXPOSURE;
    attrs[1] = xcb_generate_id(context->HostX.conn);
    xcb_create_colormap(context->HostX.conn,
                        XCB_COLORMAP_ALLOC_NONE,
                        attrs[1],
                        hostx_get_window(a_screen_number, context),
                        a_visual_id);

    win = xcb_generate_id(context->HostX.conn);
    xcb_create_window(context->HostX.conn,
                      depth,
                      win,
                      hostx_get_window(a_screen_number, context),
                      a_geometry->x, a_geometry->y,
                      a_geometry->width, a_geometry->height, 0,
                      XCB_WINDOW_CLASS_COPY_FROM_PARENT,
                      a_visual_id, winmask, attrs);

    if (scrpriv->peer_win == XCB_NONE) {
        scrpriv->peer_win = win;
    }
    else {
        EPHYR_LOG_ERROR("multiple peer windows created for same screen\n");
    }
    xcb_flush(context->HostX.conn);
    xcb_map_window(context->HostX.conn, win);
    *a_host_peer = win;
    is_ok = TRUE;
 out:
    EPHYR_LOG("leave\n");
    return is_ok;
}

int
hostx_destroy_window(int a_win, XephyrContext* context)
{
    xcb_destroy_window(context->HostX.conn, a_win);
    xcb_flush(context->HostX.conn);
    return TRUE;
}

int
hostx_set_window_geometry(int a_win, EphyrBox * a_geo, XephyrContext* context)
{
    uint32_t mask = XCB_CONFIG_WINDOW_X
                  | XCB_CONFIG_WINDOW_Y
                  | XCB_CONFIG_WINDOW_WIDTH
                  | XCB_CONFIG_WINDOW_HEIGHT;
    uint32_t values[4];

    EPHYR_RETURN_VAL_IF_FAIL(a_geo, FALSE);

    EPHYR_LOG("enter. x,y,w,h:(%d,%d,%d,%d)\n",
              a_geo->x, a_geo->y, a_geo->width, a_geo->height);

    values[0] = a_geo->x;
    values[1] = a_geo->y;
    values[2] = a_geo->width;
    values[3] = a_geo->height;
    xcb_configure_window(context->HostX.conn, a_win, mask, values);

    EPHYR_LOG("leave\n");
    return TRUE;
}

int
hostx_set_window_bounding_rectangles(int a_window,
                                     EphyrRect * a_rects, int a_num_rects, XephyrContext* context)
{
    Bool is_ok = FALSE;
    int i = 0;
    xcb_rectangle_t *rects = NULL;

    EPHYR_RETURN_VAL_IF_FAIL(a_rects, FALSE);

    EPHYR_LOG("enter. num rects:%d\n", a_num_rects);

    rects = calloc(a_num_rects, sizeof (xcb_rectangle_t));
    if (!rects)
        goto out;
    for (i = 0; i < a_num_rects; i++) {
        rects[i].x = a_rects[i].x1;
        rects[i].y = a_rects[i].y1;
        rects[i].width = abs(a_rects[i].x2 - a_rects[i].x1);
        rects[i].height = abs(a_rects[i].y2 - a_rects[i].y1);
        EPHYR_LOG("borders clipped to rect[x:%d,y:%d,w:%d,h:%d]\n",
                  rects[i].x, rects[i].y, rects[i].width, rects[i].height);
    }
    xcb_shape_rectangles(context->HostX.conn,
                         XCB_SHAPE_SO_SET,
                         XCB_SHAPE_SK_BOUNDING,
                         XCB_CLIP_ORDERING_YX_BANDED,
                         a_window,
                         0, 0,
                         a_num_rects,
                         rects);
    is_ok = TRUE;

out:
    free(rects);
    rects = NULL;
    EPHYR_LOG("leave\n");
    return is_ok;
}

#ifdef GLAMOR
Bool
ephyr_glamor_init(ScreenPtr screen)
{
    KdScreenPriv(screen);
    KdScreenInfo *kd_screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = kd_screen->driver;

    scrpriv->glamor = ephyr_glamor_glx_screen_init(scrpriv->win, screen->context);
    ephyr_glamor_set_window_size(scrpriv->glamor,
                                 scrpriv->win_width, scrpriv->win_height);

    if (!glamor_init(screen, 0)) {
        FatalError("Failed to initialize glamor\n", screen->context);
        return FALSE;
    }

    return TRUE;
}

static int
ephyrSetPixmapVisitWindow(WindowPtr window, void *data)
{
    ScreenPtr screen = window->drawable.pScreen;

    if (screen->GetWindowPixmap(window) == data) {
        screen->SetWindowPixmap(window, screen->GetScreenPixmap(screen));
        return WT_WALKCHILDREN;
    }
    return WT_DONTWALKCHILDREN;
}

Bool
ephyr_glamor_create_screen_resources(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *kd_screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = kd_screen->driver;
    PixmapPtr old_screen_pixmap, screen_pixmap;
    uint32_t tex;

    XephyrContext* context = pScreen->context;
    if (!context->ephyr_glamor)
        return TRUE;

    /* kdrive's fbSetupScreen() told mi to have
     * miCreateScreenResources() (which is called before this) make a
     * scratch pixmap wrapping ephyr-glamor's NULL
     * KdScreenInfo->fb.framebuffer.
     *
     * We want a real (texture-based) screen pixmap at this point.
     * This is what glamor will render into, and we'll then texture
     * out of that into the host's window to present the results.
     *
     * Thus, delete the current screen pixmap, and put a fresh one in.
     */
    old_screen_pixmap = pScreen->GetScreenPixmap(pScreen);
    pScreen->DestroyPixmap(old_screen_pixmap);

    screen_pixmap = pScreen->CreatePixmap(pScreen,
                                          pScreen->width,
                                          pScreen->height,
                                          pScreen->rootDepth,
                                          GLAMOR_CREATE_NO_LARGE);
    if (!screen_pixmap)
        return FALSE;

    pScreen->SetScreenPixmap(screen_pixmap);
    if (pScreen->root && pScreen->SetWindowPixmap)
        TraverseTree(pScreen->root, ephyrSetPixmapVisitWindow, old_screen_pixmap);

    /* Tell the GLX code what to GL texture to read from. */
    tex = glamor_get_pixmap_texture(screen_pixmap);
    if (!tex)
        return FALSE;

    ephyr_glamor_set_texture(scrpriv->glamor, tex);

    return TRUE;
}

void
ephyr_glamor_enable(ScreenPtr screen)
{
}

void
ephyr_glamor_disable(ScreenPtr screen)
{
}

void
ephyr_glamor_fini(ScreenPtr screen)
{
    KdScreenPriv(screen);
    KdScreenInfo *kd_screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = kd_screen->driver;

    glamor_fini(screen);
    ephyr_glamor_glx_screen_fini(scrpriv->glamor);
    scrpriv->glamor = NULL;
}
#endif
