#include "dix/context.h"
/* Forward declaration for InitOutput from ephyr layer */
extern void InitOutput(ScreenInfo *pScreenInfo, int argc, char **argv, XephyrContext* context);
/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xos.h>            /* for unistd.h  */
#include <X11/Xproto.h>
#include <pixman.h>
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "extension.h"
#include "colormap.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "selection.h"
#include <X11/fonts/font.h>
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/libxfont2.h>
#include "opaque.h"
#include "servermd.h"
#include "hotplug.h"
#include "dixfont.h"
#include "extnsionst.h"
#include "privates.h"
#include "registry.h"
#include "client.h"
#include "exevents.h"
#ifdef PANORAMIX
#include "panoramiXsrv.h"
#else
#include "dixevents.h"          /* InitEvents() */
#endif

#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#include "dpmsproc.h"
#endif

extern void Dispatch(XephyrContext* context);

CallbackListPtr RootWindowFinalizeCallback = NULL;



int
dix_main(int argc, char *argv[], char *envp[], XephyrContext* context)
{
    int i;
    HWEventQueueType alwaysCheckForInput[2];

    /* Context should be provided by caller */
    if (!context) {
        FatalError("No context provided - server not properly initialized", context);
    }
    
    

    context->display = "0";

    InitRegions();

    CheckUserParameters(argc, argv, envp, context);

    CheckUserAuthorization(context);

    ProcessCommandLine(argc, argv, context);

    alwaysCheckForInput[0] = 0;
    alwaysCheckForInput[1] = 1;
    while (1) {
        context->serverGeneration++;
        context->ScreenSaverTime = context->defaultScreenSaverTime;
        context->ScreenSaverInterval = context->defaultScreenSaverInterval;
        context->ScreenSaverBlanking = context->defaultScreenSaverBlanking;
        context->ScreenSaverAllowExposures = context->defaultScreenSaverAllowExposures;

        InitBlockAndWakeupHandlers();
        /* Perform any operating system dependent initializations you'd like */
        OsInit(context);
        if (context->serverGeneration == 1) {
            CreateWellKnownSockets(context);
            for (i = 1; i < LimitClients; i++)
                context->clients[i] = NullClient;
            context->serverClient = calloc(sizeof(ClientRec), 1);
            if (!context->serverClient)
                FatalError("couldn't create server client", context);
            InitClient(context->serverClient, 0, (void *) NULL);
            context->serverClient->context = context;
        }
        else
            ResetWellKnownSockets(context);
        context->clients[0] = context->serverClient;
        context->currentMaxClients = 1;

        /* clear any existing selections */
        InitSelections(context);

        /* Initialize privates before first allocation */
        dixResetPrivates(context);

        /* Initialize server client devPrivates, to be reallocated as
         * more client privates are registered
         */
        if (!dixAllocatePrivates(&context->serverClient->devPrivates, PRIVATE_CLIENT))
            FatalError("failed to create server client privates", context);

        if (!InitClientResources(context->serverClient)) /* for root resources */
            FatalError("couldn't init server resources");

        SetInputCheck(&alwaysCheckForInput[0], &alwaysCheckForInput[1], context);
        context->screenInfo.numScreens = 0;

        InitAtoms(context);
        InitEvents(context);
        xfont2_init_glyph_caching();
        dixResetRegistry();
        InitFonts();
        InitCallbackManager();
        InitOutput(&context->screenInfo, argc, argv, context);

        if (context->screenInfo.numScreens < 1)
            FatalError("no screens found", context);
        InitExtensions(argc, argv, context);

        for (i = 0; i < context->screenInfo.numGPUScreens; i++) {
            ScreenPtr pScreen = context->screenInfo.gpuscreens[i];
            if (!CreateScratchPixmapsForScreen(pScreen))
                FatalError("failed to create scratch pixmaps", context);
            if (pScreen->CreateScreenResources &&
                !(*pScreen->CreateScreenResources) (pScreen))
                FatalError("failed to create screen resources");
        }

        for (i = 0; i < context->screenInfo.numScreens; i++) {
            ScreenPtr pScreen = context->screenInfo.screens[i];

            if (!CreateScratchPixmapsForScreen(pScreen))
                FatalError("failed to create scratch pixmaps", context);
            if (pScreen->CreateScreenResources &&
                !(*pScreen->CreateScreenResources) (pScreen))
                FatalError("failed to create screen resources");
            if (!CreateGCperDepth(i, context))
                FatalError("failed to create scratch GCs", context);
            if (!CreateDefaultStipple(i))
                FatalError("failed to create default stipple", context);
            if (!CreateRootWindow(pScreen))
                FatalError("failed to create root window");
            CallCallbacks(&RootWindowFinalizeCallback, pScreen);
        }

        if (SetDefaultFontPath(context->defaultFontPath, context) != Success) {
            ErrorF("[dix] failed to set default font path '%s'", context->defaultFontPath);
        }
        if (!SetDefaultFont("fixed", context)) {
            FatalError("could not open default font", context);
        }

        if (!(context->rootCursor = CreateRootCursor(NULL, 0, context))) {
            FatalError("could not open default cursor font", context);
        }

#ifdef PANORAMIX
        /*
         * Consolidate window and colourmap information for each screen
         */
        if (!noPanoramiXExtension)
            PanoramiXConsolidate(context);
#endif

        for (i = 0; i < context->screenInfo.numScreens; i++)
            InitRootWindow(context->screenInfo.screens[i]->root);

        InitCoreDevices(context);
        InitInput(argc, argv, context);
        InitAndStartDevices(context);
        ReserveClientIds(context->serverClient);

        dixSaveScreens(context->serverClient, SCREEN_SAVER_FORCER, ScreenSaverReset);

        dixCloseRegistry();

#ifdef PANORAMIX
        if (!noPanoramiXExtension) {
            if (!PanoramiXCreateConnectionBlock(context)) {
                FatalError("could not create connection block info", context);
            }
        }
        else
#endif
        {
            if (!CreateConnectionBlock(context)) {
                FatalError("could not create connection block info");
            }
        }

        NotifyParentProcess(context);

        InputThreadInit();

        /* Signal libxephyr that server is ready (if available) */
        extern void xephyr_signal_ready(void) __attribute__((weak));
        if (xephyr_signal_ready) {
            xephyr_signal_ready();
        }

        Dispatch(context);

        UndisplayDevices(context);
        DisableAllDevices(context);

        /* Now free up whatever must be freed */
        if (screenIsSaved == SCREEN_SAVER_ON)
            dixSaveScreens(context->serverClient, SCREEN_SAVER_OFF, ScreenSaverReset);
        FreeScreenSaverTimer();
        CloseDownExtensions();

#ifdef PANORAMIX
        {
            Bool remember_it = noPanoramiXExtension;

            noPanoramiXExtension = TRUE;
            FreeAllResources(context);
            noPanoramiXExtension = remember_it;
        }
#else
        FreeAllResources(context);
#endif

        CloseInput(context);

        InputThreadFini();

        for (i = 0; i < context->screenInfo.numScreens; i++)
            context->screenInfo.screens[i]->root = NullWindow;

        CloseDownDevices(context);

        CloseDownEvents();

        for (i = context->screenInfo.numGPUScreens - 1; i >= 0; i--) {
            ScreenPtr pScreen = context->screenInfo.gpuscreens[i];
            FreeScratchPixmapsForScreen(pScreen);
            dixFreeScreenSpecificPrivates(pScreen);
            (*pScreen->CloseScreen) (pScreen);
            dixFreePrivates(pScreen->devPrivates, PRIVATE_SCREEN);
            free(pScreen);
            context->screenInfo.numGPUScreens = i;
        }

        for (i = context->screenInfo.numScreens - 1; i >= 0; i--) {
            FreeScratchPixmapsForScreen(context->screenInfo.screens[i]);
            FreeGCperDepth(i, context);
            FreeDefaultStipple(i, context);
            dixFreeScreenSpecificPrivates(context->screenInfo.screens[i]);
            (*context->screenInfo.screens[i]->CloseScreen) (context->screenInfo.screens[i]);
            dixFreePrivates(context->screenInfo.screens[i]->devPrivates, PRIVATE_SCREEN);
            free(context->screenInfo.screens[i]);
            context->screenInfo.numScreens = i;
        }

        ReleaseClientIds(context->serverClient);
        dixFreePrivates(context->serverClient->devPrivates, PRIVATE_CLIENT);
        context->serverClient->devPrivates = NULL;

	dixFreeRegistry();

        FreeFonts();

        FreeAllAtoms();

        FreeAuditTimer();

        DeleteCallbackManager();

        ClearWorkQueue(context);

        if (dispatchException & DE_TERMINATE) {
            CloseWellKnownConnections();
        }

        OsCleanup((dispatchException & DE_TERMINATE) != 0);

        if (dispatchException & DE_TERMINATE) {
            ddxGiveUp(EXIT_NO_ERROR);
            break;
        }

        free(context->ConnectionInfo);
        context->ConnectionInfo = NULL;
    }
    return 0;
}
