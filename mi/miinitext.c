#include "dix/context.h"
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

/*
 * Copyright (c) 2000 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#include "xf86Extensions.h"
#endif

#ifdef HAVE_DMX_CONFIG_H
#include <dmx-config.h>
#undef XV
#undef DBE
#undef SCREENSAVER
#undef RANDR
#undef DAMAGE
#undef COMPOSITE
#undef MITSHM
#endif

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#undef COMPOSITE
#undef DPMSExtension
#endif

#include "misc.h"
#include "extension.h"
#include "extinit.h"
#include "micmap.h"
#include "os.h"
#include "globals.h"

#include "miinitext.h"

/* Wrapper functions for extensions that don't take XephyrContext* yet */
static void ShapeExtensionInitWrapper(XephyrContext* context) {
    ShapeExtensionInit(context);
}

static void XCMiscExtensionInitWrapper(XephyrContext* context) {
    XCMiscExtensionInit(context);
}

static void BigReqExtensionInitWrapper(XephyrContext* context) {
    BigReqExtensionInit(context);
}

#ifdef XCSECURITY
static void SecurityExtensionInitWrapper(XephyrContext* context) {
    SecurityExtensionInit(context);
}
#endif

#ifdef XF86BIGFONT
static void XFree86BigfontExtensionInitWrapper(XephyrContext* context) {
    XFree86BigfontExtensionInit();
}
#endif

#ifdef XTEST
static void XTestExtensionInitWrapper(XephyrContext* context) {
    XTestExtensionInit(context);
}
#endif

#ifdef RES
static void ResExtensionInitWrapper(XephyrContext* context) {
    ResExtensionInit(context);
}
#endif

#ifdef XV
static void XvMCExtensionInitWrapper(XephyrContext* context) {
    XvMCExtensionInit(context);
}
#endif

#ifdef XSELINUX
static void SELinuxExtensionInitWrapper(XephyrContext* context) {
    SELinuxExtensionInit(context);
}
#endif

#ifdef DRI3
static void dri3_extension_init_wrapper(XephyrContext* context) {
    dri3_extension_init();
}
#endif

/* Global pointer for dynamically set extension disable flags */
static Bool *noTestExtensionsPtr = NULL;

/* List of built-in (statically linked) extensions */
static ExtensionModule staticExtensions[] = {
    {GEExtensionInit, "Generic Event Extension", NULL}, /* will be set to &context->noGEExtension at runtime */
    {ShapeExtensionInitWrapper, "SHAPE", NULL},
#ifdef MITSHM
    {ShmExtensionInit, "MIT-SHM", NULL}, /* will be set to &context->noMITShmExtension at runtime */
#endif
    {XInputExtensionInit, "XInputExtension", NULL},
#ifdef XTEST
    {XTestExtensionInitWrapper, "XTEST", NULL},
#endif
    {BigReqExtensionInitWrapper, "BIG-REQUESTS", NULL},
    {SyncExtensionInit, "SYNC", NULL},
    {XkbExtensionInit, "XKEYBOARD", NULL},
    {XCMiscExtensionInitWrapper, "XC-MISC", NULL},
#ifdef XCSECURITY
    {SecurityExtensionInitWrapper, "SECURITY", &noSecurityExtension},
#endif
#ifdef PANORAMIX
    {PanoramiXExtensionInit, "XINERAMA", &noPanoramiXExtension},
#endif
    /* must be before Render to layer DisplayCursor correctly */
    {XFixesExtensionInit, "XFIXES", NULL}, /* will be set to &context->noXFixesExtension at runtime */
#ifdef XF86BIGFONT
    {XFree86BigfontExtensionInitWrapper, "XFree86-Bigfont", &noXFree86BigfontExtension},
#endif
    {RenderExtensionInit, "RENDER", NULL}, /* will be set to &context->noRenderExtension at runtime */
#ifdef RANDR
    {RRExtensionInit, "RANDR", NULL}, /* will be set to &context->noRRExtension at runtime */
#endif
#ifdef COMPOSITE
    {CompositeExtensionInit, "COMPOSITE", NULL},
#endif
#ifdef DAMAGE
    {DamageExtensionInit, "DAMAGE", NULL},
#endif
#ifdef SCREENSAVER
    {ScreenSaverExtensionInit, "MIT-SCREEN-SAVER", NULL}, /* will be set to &context->noScreenSaverExtension at runtime */
#endif
#ifdef DBE
    {DbeExtensionInit, "DOUBLE-BUFFER", NULL},
#endif
#ifdef XRECORD
    {RecordExtensionInit, "RECORD", NULL},
#endif
#ifdef DPMSExtension
    {DPMSExtensionInit, "DPMS", &noDPMSExtension},
#endif
#ifdef PRESENT
    {present_extension_init, "Present", NULL},
#endif
#ifdef DRI3
    {dri3_extension_init_wrapper, "DRI3", NULL},
#endif
#ifdef RES
    {ResExtensionInitWrapper, "X-Resource", NULL}, /* will be set to &context->noResExtension at runtime */
#endif
#ifdef XV
    {XvExtensionInit, "XVideo", NULL}, /* will be set to &context->noXvExtension at runtime */
    {XvMCExtensionInitWrapper, "XVideo-MotionCompensation", NULL}, /* will be set to &context->noXvExtension at runtime */
#endif
#ifdef XSELINUX
    {SELinuxExtensionInitWrapper, "SELinux", &noSELinuxExtension},
#endif
#ifdef GLXEXT
    {GlxExtensionInit, "GLX", NULL}, /* will be set to &context->noGlxExtension at runtime */
#endif
};

void
ListStaticExtensions(XephyrContext* context)
{
    const ExtensionModule *ext;
    int i;

    ErrorF(" Only the following extensions can be run-time enabled/disabled:\n", context);
    for (i = 0; i < ARRAY_SIZE(staticExtensions); i++) {
        ext = &staticExtensions[i];
        if (ext->disablePtr != NULL) {
            ErrorF("\t%s\n", context, ext->name);
        }
    }
}

Bool
EnableDisableExtension(const char *name, Bool enable)
{
    const ExtensionModule *ext;
    int i;

    for (i = 0; i < ARRAY_SIZE(staticExtensions); i++) {
        ext = &staticExtensions[i];
        if (strcasecmp(name, ext->name) == 0) {
            if (ext->disablePtr != NULL) {
                *ext->disablePtr = !enable;
                return TRUE;
            }
            else {
                /* Extension is always on, impossible to disable */
                return enable;  /* okay if they wanted to enable,
                                   fail if they tried to disable */
            }
        }
    }

    return FALSE;
}

void
EnableDisableExtensionError(const char *name, Bool enable, XephyrContext* context)
{
    const ExtensionModule *ext;
    int i;
    Bool found = FALSE;

    for (i = 0; i < ARRAY_SIZE(staticExtensions); i++) {
        ext = &staticExtensions[i];
        if ((strcmp(name, ext->name) == 0) && (ext->disablePtr == NULL)) {
            ErrorF("[mi] Extension \"%s\" can not be disabled\n", context, name);
            found = TRUE;
            break;
        }
    }
    if (found == FALSE) {
        ErrorF("[mi] Extension \"%s\" is not recognized\n", context, name);
        /* Disabling a non-existing extension is a no-op anyway */
        if (enable == FALSE)
            return;
    }
    ListStaticExtensions(context);
}

// REMOVED: static ExtensionModule *context->ExtensionModuleList = NULL; - moved to XephyrContext
static int numExtensionModules = 0;

static void
UpdateExtensionPointers(XephyrContext* context)
{
    ExtensionModule *ext;
    int i;

    /* Set up runtime pointers for context-based flags */
    if (!noTestExtensionsPtr && context) {
        noTestExtensionsPtr = &context->noTestExtensions;
    }

    /* Update extension module pointers to use context-based flags */
    for (i = 0; i < ARRAY_SIZE(staticExtensions); i++) {
        ext = (ExtensionModule *)&staticExtensions[i];
        
        /* Update pointers based on extension name */
        if (strcmp(ext->name, "Generic Event Extension") == 0) {
            ext->disablePtr = &context->noGEExtension;
        }
#ifdef MITSHM
        else if (strcmp(ext->name, "MIT-SHM") == 0) {
            ext->disablePtr = &context->noMITShmExtension;
        }
#endif
        else if (strcmp(ext->name, "XFIXES") == 0) {
            ext->disablePtr = &context->noXFixesExtension;
        }
        else if (strcmp(ext->name, "RENDER") == 0) {
            ext->disablePtr = &context->noRenderExtension;
        }
#ifdef RANDR
        else if (strcmp(ext->name, "RANDR") == 0) {
            ext->disablePtr = &context->noRRExtension;
        }
#endif
#ifdef SCREENSAVER
        else if (strcmp(ext->name, "MIT-SCREEN-SAVER") == 0) {
            ext->disablePtr = &context->noScreenSaverExtension;
        }
#endif
#ifdef RES
        else if (strcmp(ext->name, "X-Resource") == 0) {
            ext->disablePtr = &context->noResExtension;
        }
#endif
#ifdef XV
        else if (strcmp(ext->name, "XVideo") == 0 || 
                 strcmp(ext->name, "XVideo-MotionCompensation") == 0) {
            ext->disablePtr = &context->noXvExtension;
        }
#endif
#ifdef GLXEXT
        else if (strcmp(ext->name, "GLX") == 0) {
            ext->disablePtr = &context->noGlxExtension;
        }
#endif
#ifdef XTEST
        else if (strcmp(ext->name, "XTEST") == 0) {
            ext->disablePtr = noTestExtensionsPtr;
        }
#endif
#ifdef XRECORD
        else if (strcmp(ext->name, "RECORD") == 0) {
            ext->disablePtr = noTestExtensionsPtr;
        }
#endif
    }
}

static void
AddStaticExtensions(XephyrContext* context)
{
    static Bool listInitialised = FALSE;

    if (listInitialised)
        return;
    listInitialised = TRUE;

    /* Update extension pointers to use context-based flags */
    UpdateExtensionPointers(context);

    /* Add built-in extensions to the list. */
    LoadExtensionList(staticExtensions, ARRAY_SIZE(staticExtensions), TRUE, context);
}

void
InitExtensions(int argc, char *argv[], XephyrContext* context)
{
    int i;
    ExtensionModule *ext;

    AddStaticExtensions(context);

    for (i = 0; i < numExtensionModules; i++) {
        ext = &context->ExtensionModuleList[i];
        if (ext->initFunc != NULL &&
            (ext->disablePtr == NULL || !*ext->disablePtr)) {
            LogMessageVerb(X_INFO, 3, "Initializing extension %s\n", context,
                           ext->name);

            (ext->initFunc) (context);
        }
    }
}

static ExtensionModule *
NewExtensionModuleList(int size, XephyrContext* context)
{
    ExtensionModule *save = context->ExtensionModuleList;
    int n;

    /* Sanity check */
    if (!context->ExtensionModuleList)
        numExtensionModules = 0;

    n = numExtensionModules + size;
    context->ExtensionModuleList = reallocarray(context->ExtensionModuleList, n,
                                       sizeof(ExtensionModule));
    if (context->ExtensionModuleList == NULL) {
        context->ExtensionModuleList = save;
        return NULL;
    }
    else {
        numExtensionModules += size;
        return context->ExtensionModuleList + (numExtensionModules - size);
    }
}

void
LoadExtensionList(const ExtensionModule ext[], int size, Bool builtin, XephyrContext* context)
{
    ExtensionModule *newext;
    int i;

    /* Make sure built-in extensions get added to the list before those
     * in modules. */
    AddStaticExtensions(context);

    if (!(newext = NewExtensionModuleList(size, context)))
        return;

    for (i = 0; i < size; i++, newext++) {
        newext->name = ext[i].name;
        newext->initFunc = ext[i].initFunc;
        newext->disablePtr = ext[i].disablePtr;
    }
}
