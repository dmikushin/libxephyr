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
#include "ephyr.h"
#include "ephyrlog.h"
#include "glx_extinit.h"

extern Window EphyrPreExistingHostWin;
/* kdHasPointer, kdHasKbd moved to XephyrContext */

void processScreenOrOutputArg(const char *screen_size, const char *output, char *parent_id, XephyrContext* context);
void processOutputArg(const char *output, char *parent_id, XephyrContext* context);
void processScreenArg(const char *screen_size, char *parent_id, XephyrContext* context);

void
InitCard(char *name)
{
    EPHYR_DBG("mark");
    extern XephyrContext *kdGlobalContext;
    if (kdGlobalContext)
        KdCardInfoAdd(&ephyrFuncs, 0, kdGlobalContext);
}

void
InitOutput(ScreenInfo * pScreenInfo, int argc, char **argv, XephyrContext* context)
{
    KdInitOutput(pScreenInfo, argc, argv, context);
}

void
InitInput(int argc, char **argv, XephyrContext* context)
{
    KdKeyboardInfo *ki;
    KdPointerInfo *pi;

    if (!context->SeatId) {
        KdAddKeyboardDriver(&EphyrKeyboardDriver);
        KdAddPointerDriver(&EphyrMouseDriver);

        if (!context->kdHasKbd) {
            ki = KdNewKeyboard(context);
            if (!ki)
                FatalError("Couldn't create Xephyr keyboard\n", context);
            ki->driver = &EphyrKeyboardDriver;
            ki->context = context;
            KdAddKeyboard(ki, context);
        }

        if (!context->kdHasPointer) {
            pi = KdNewPointer();
            if (!pi)
                FatalError("Couldn't create Xephyr pointer\n", context);
            pi->driver = &EphyrMouseDriver;
            pi->context = context;
            KdAddPointer(pi, context);
        }
    }

    KdInitInput(context);
}

void
CloseInput(XephyrContext* context)
{
    KdCloseInput(context);
}

#if INPUTTHREAD
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void)
{
}
#endif

#ifdef DDXBEFORERESET
void
ddxBeforeReset(void)
{
}
#endif

void
ddxUseMsg(XephyrContext* context)
{
    KdUseMsg(context);

    ErrorF("\nXephyr Option Usage:\n", context);
    ErrorF("-parent <XID>        Use existing window as Xephyr root win\n", context);
    ErrorF("-sw-cursor           Render cursors in software in Xephyr\n", context);
    ErrorF("-fullscreen          Attempt to run Xephyr fullscreen\n", context);
    ErrorF("-output <NAME>       Attempt to run Xephyr fullscreen (restricted to given output geometry)\n", context);
    ErrorF("-grayscale           Simulate 8bit grayscale\n", context);
    ErrorF("-resizeable          Make Xephyr windows resizeable\n", context);
#ifdef GLAMOR
    ErrorF("-glamor              Enable 2D acceleration using glamor\n", context);
    ErrorF("-glamor_gles2        Enable 2D acceleration using glamor (with GLES2 only)\n", context);
    ErrorF("-glamor-skip-present Skip presenting the output when using glamor (for internal testing optimization)\n", context);
#endif
    ErrorF
        ("-fakexa              Simulate acceleration using software rendering\n", context);
    ErrorF("-verbosity <level>   Set log verbosity level\n", context);
    ErrorF("-noxv                do not use XV\n", context);
    ErrorF("-name [name]         define the name in the WM_CLASS property\n", context);
    ErrorF
        ("-title [title]       set the window title in the WM_NAME property\n", context);
    ErrorF("-no-host-grab        Disable grabbing the keyboard and mouse.\n", context);
    ErrorF("\n", context);
}

void
processScreenOrOutputArg(const char *screen_size, const char *output, char *parent_id, XephyrContext* context)
{
    KdCardInfo *card;

    InitCard(0);                /*Put each screen on a separate card */
    card = KdCardInfoLast(context);

    if (card) {
        KdScreenInfo *screen;
        unsigned long p_id = 0;
        Bool use_geometry;

        screen = KdScreenInfoAdd(card);
        KdParseScreen(screen, screen_size, context);
        screen->driver = calloc(1, sizeof(EphyrScrPriv));
        if (!screen->driver)
            FatalError("Couldn't alloc screen private\n", context);

        if (parent_id) {
            p_id = strtol(parent_id, NULL, 0);
        }

        use_geometry = (strchr(screen_size, '+') != NULL);
        EPHYR_DBG("screen number:%d\n", screen->mynum);
        hostx_add_screen(screen, p_id, screen->mynum, use_geometry, output, context);
    }
    else {
        ErrorF("No matching card found!\n", context);
    }
}

void
processScreenArg(const char *screen_size, char *parent_id, XephyrContext* context)
{
    processScreenOrOutputArg(screen_size, NULL, parent_id, context);
}

void
processOutputArg(const char *output, char *parent_id, XephyrContext* context)
{
    processScreenOrOutputArg("100x100+0+0", output, parent_id, context);
}

int
ddxProcessArgument(int argc, char **argv, int i, XephyrContext* context)
{
    static char *parent = NULL;

    EPHYR_DBG("mark argv[%d]='%s'", i, argv[i]);

    if (!strcmp(argv[i], "-parent")) {
        if (i + 1 < argc) {
            int j;

            /* If parent is specified and a screen argument follows, don't do
             * anything, let the -screen handling init the rest */
            for (j = i; j < argc; j++) {
                if (!strcmp(argv[j], "-screen")) {
                    parent = argv[i + 1];
                    return 2;
                }
            }

            processScreenArg("100x100", argv[i + 1], context);
            return 2;
        }

        UseMsg(context);
        exit(1);
    }
    else if (!strcmp(argv[i], "-screen")) {
        if ((i + 1) < argc) {
            processScreenArg(argv[i + 1], parent, context);
            parent = NULL;
            return 2;
        }

        UseMsg(context);
        exit(1);
    }
    else if (!strcmp(argv[i], "-output")) {
        if (i + 1 < argc) {
            processOutputArg(argv[i + 1], NULL, context);
            return 2;
        }

        UseMsg(context);
        exit(1);
    }
    else if (!strcmp(argv[i], "-sw-cursor")) {
        hostx_use_sw_cursor();
        return 1;
    }
    else if (!strcmp(argv[i], "-host-cursor")) {
        /* Compatibility with the old command line argument, now the default. */
        return 1;
    }
    else if (!strcmp(argv[i], "-fullscreen")) {
        hostx_use_fullscreen();
        return 1;
    }
    else if (!strcmp(argv[i], "-grayscale")) {
        context->EphyrWantGrayScale = 1;
        return 1;
    }
    else if (!strcmp(argv[i], "-resizeable")) {
        context->EphyrWantResize = 1;
        return 1;
    }
#ifdef GLAMOR
    else if (!strcmp (argv[i], "-glamor")) {
        context->ephyr_glamor = TRUE;
        ephyrFuncs.initAccel = ephyr_glamor_init;
        ephyrFuncs.enableAccel = ephyr_glamor_enable;
        ephyrFuncs.disableAccel = ephyr_glamor_disable;
        ephyrFuncs.finiAccel = ephyr_glamor_fini;
        return 1;
    }
    else if (!strcmp (argv[i], "-glamor_gles2")) {
        context->ephyr_glamor = TRUE;
        context->ephyr_glamor_gles2 = TRUE;
        ephyrFuncs.initAccel = ephyr_glamor_init;
        ephyrFuncs.enableAccel = ephyr_glamor_enable;
        ephyrFuncs.disableAccel = ephyr_glamor_disable;
        ephyrFuncs.finiAccel = ephyr_glamor_fini;
        return 1;
    }
    else if (!strcmp (argv[i], "-glamor-skip-present")) {
        context->ephyr_glamor_skip_present = TRUE;
        return 1;
    }
#endif
    else if (!strcmp(argv[i], "-fakexa")) {
        ephyrFuncs.initAccel = ephyrDrawInit;
        ephyrFuncs.enableAccel = ephyrDrawEnable;
        ephyrFuncs.disableAccel = ephyrDrawDisable;
        ephyrFuncs.finiAccel = ephyrDrawFini;
        return 1;
    }
    else if (!strcmp(argv[i], "-verbosity")) {
        if (i + 1 < argc && argv[i + 1][0] != '-') {
            int verbosity = atoi(argv[i + 1]);

            LogSetParameter(XLOG_VERBOSITY, verbosity);
            EPHYR_LOG("set verbosiry to %d\n", verbosity);
            return 2;
        }
        else {
            UseMsg(context);
            exit(1);
        }
    }
    else if (!strcmp(argv[i], "-noxv")) {
        context->ephyrNoXV = TRUE;
        EPHYR_LOG("no XVideo enabled\n");
        return 1;
    }
    else if (!strcmp(argv[i], "-name")) {
        if (i + 1 < argc && argv[i + 1][0] != '-') {
            hostx_use_resname(argv[i + 1], 1, context);
            return 2;
        }
        else {
            UseMsg(context);
            return 0;
        }
    }
    else if (!strcmp(argv[i], "-title")) {
        if (i + 1 < argc && argv[i + 1][0] != '-') {
            hostx_set_title(argv[i + 1], context);
            return 2;
        }
        else {
            UseMsg(context);
            return 0;
        }
    }
    else if (argv[i][0] == ':') {
        hostx_set_display_name(argv[i]);
    }
    /* Xnest compatibility */
    else if (!strcmp(argv[i], "-context->display")) {
        hostx_set_display_name(argv[i + 1]);
        return 2;
    }
    else if (!strcmp(argv[i], "-sync") ||
             !strcmp(argv[i], "-full") ||
             !strcmp(argv[i], "-sss") || !strcmp(argv[i], "-install")) {
        return 1;
    }
    else if (!strcmp(argv[i], "-bw") ||
             !strcmp(argv[i], "-class") ||
             !strcmp(argv[i], "-geometry") || !strcmp(argv[i], "-scrns")) {
        return 2;
    }
    /* end Xnest compat */
    else if (!strcmp(argv[i], "-no-host-grab")) {
        context->EphyrWantNoHostGrab = 1;
        return 1;
    }
    else if (!strcmp(argv[i], "-sharevts") ||
             !strcmp(argv[i], "-novtswitch")) {
        return 1;
    }
    else if (!strcmp(argv[i], "-layout")) {
        return 2;
    }

    return KdProcessArgument(argc, argv, i, context);
}

void
OsVendorInit(XephyrContext* context)
{
    EPHYR_DBG("mark");

    if (context->SeatId)
        hostx_use_sw_cursor();

    if (hostx_want_host_cursor())
        ephyrFuncs.initCursor = &ephyrCursorInit;

    if (context->serverGeneration == 1) {
        if (!KdCardInfoLast(context)) {
            processScreenArg("640x480", NULL, context);
        }
        hostx_init(context);
    }
}

KdCardFuncs ephyrFuncs = {
    ephyrCardInit,              /* cardinit */
    ephyrScreenInitialize,      /* scrinit */
    ephyrInitScreen,            /* initScreen */
    ephyrFinishInitScreen,      /* finishInitScreen */
    ephyrCreateResources,       /* createRes */
    ephyrScreenFini,            /* scrfini */
    ephyrCardFini,              /* cardfini */

    0,                          /* initCursor */

    0,                          /* initAccel */
    0,                          /* enableAccel */
    0,                          /* disableAccel */
    0,                          /* finiAccel */

    ephyrGetColors,             /* getColors */
    ephyrPutColors,             /* putColors */

    ephyrCloseScreen,           /* closeScreen */
};
