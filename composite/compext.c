#include "dix/context.h"
/*
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "compint.h"
#include "xace.h"
#include "protocol-versions.h"
#include "extinit.h"

//static CARD8 CompositeReqCode;
// static DevPrivateKeyRec CompositeClientPrivateKeyRec;

#define CompositeClientPrivateKey(client) (&(client)->context->CompositeClientPrivateKeyRec)

typedef struct _CompositeClient {
    int major_version;
    int minor_version;
} CompositeClientRec, *CompositeClientPtr;

#define GetCompositeClient(pClient) ((CompositeClientPtr) \
    dixLookupPrivate(&(pClient)->devPrivates, CompositeClientPrivateKey(pClient)))

static int
FreeCompositeClientWindow(void *value, XID ccwid, XephyrContext* context)
{
    WindowPtr pWin = value;

    compFreeClientWindow(pWin, ccwid);
    return Success;
}

static int
FreeCompositeClientSubwindows(void *value, XID ccwid, XephyrContext* context)
{
    WindowPtr pWin = value;

    compFreeClientSubwindows(pWin, ccwid);
    return Success;
}

static int
FreeCompositeClientOverlay(void *value, XID ccwid, XephyrContext* context)
{
    CompOverlayClientPtr pOc = (CompOverlayClientPtr) value;

    compFreeOverlayClient(pOc);
    return Success;
}

static int
ProcCompositeQueryVersion(ClientPtr client)
{
    CompositeClientPtr pCompositeClient = GetCompositeClient(client);
    xCompositeQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    REQUEST(xCompositeQueryVersionReq);

    REQUEST_SIZE_MATCH(xCompositeQueryVersionReq);
    if (stuff->majorVersion < SERVER_COMPOSITE_MAJOR_VERSION) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }
    else {
        rep.majorVersion = SERVER_COMPOSITE_MAJOR_VERSION;
        rep.minorVersion = SERVER_COMPOSITE_MINOR_VERSION;
    }
    pCompositeClient->major_version = rep.majorVersion;
    pCompositeClient->minor_version = rep.minorVersion;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xCompositeQueryVersionReply), &rep);
    return Success;
}

#define VERIFY_WINDOW(pWindow, wid, client, mode)			\
    do {								\
	int err;							\
	err = dixLookupResourceByType((void **) &pWindow, wid,	\
				      RT_WINDOW, client, mode, client->context);		\
	if (err != Success) {						\
	    client->errorValue = wid;					\
	    return err;							\
	}								\
    } while (0)

static int
ProcCompositeRedirectWindow(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xCompositeRedirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compRedirectWindow(client, pWin, stuff->update);
}

static int
ProcCompositeRedirectSubwindows(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xCompositeRedirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compRedirectSubwindows(client, pWin, stuff->update);
}

static int
ProcCompositeUnredirectWindow(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xCompositeUnredirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compUnredirectWindow(client, pWin, stuff->update);
}

static int
ProcCompositeUnredirectSubwindows(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xCompositeUnredirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);

    return compUnredirectSubwindows(client, pWin, stuff->update);
}

static int
ProcCompositeCreateRegionFromBorderClip(ClientPtr client)
{
    WindowPtr pWin;
    CompWindowPtr cw;
    RegionPtr pBorderClip, pRegion;

    REQUEST(xCompositeCreateRegionFromBorderClipReq);

    REQUEST_SIZE_MATCH(xCompositeCreateRegionFromBorderClipReq);
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    LEGAL_NEW_RESOURCE(stuff->region, client);

    cw = GetCompWindow(pWin);
    if (cw)
        pBorderClip = &cw->borderClip;
    else
        pBorderClip = &pWin->borderClip;
    pRegion = XFixesRegionCopy(pBorderClip, client->context);
    if (!pRegion)
        return BadAlloc;
    RegionTranslate(pRegion, -pWin->drawable.x, -pWin->drawable.y);

    if (!AddResource(stuff->region, client->context->RegionResType, (void *) pRegion, client->context))
        return BadAlloc;

    return Success;
}

static int
ProcCompositeNameWindowPixmap(ClientPtr client)
{
    WindowPtr pWin;
    CompWindowPtr cw;
    PixmapPtr pPixmap;
    ScreenPtr pScreen;
    int rc;

    REQUEST(xCompositeNameWindowPixmapReq);

    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);

    pScreen = pWin->drawable.pScreen;

    if (!pWin->viewable)
        return BadMatch;

    LEGAL_NEW_RESOURCE(stuff->pixmap, client);

    cw = GetCompWindow(pWin);
    if (!cw)
        return BadMatch;

    pPixmap = (*pScreen->GetWindowPixmap) (pWin);
    if (!pPixmap)
        return BadMatch;

    /* security creation/labeling check */
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pixmap, RT_PIXMAP,
                  pPixmap, RT_WINDOW, pWin, DixCreateAccess);
    if (rc != Success)
        return rc;

    ++pPixmap->refcnt;

    if (!AddResource(stuff->pixmap, RT_PIXMAP, (void *) pPixmap, client->context))
        return BadAlloc;

    if (pScreen->NameWindowPixmap) {
        rc = pScreen->NameWindowPixmap(pWin, pPixmap, stuff->pixmap);
        if (rc != Success) {
            FreeResource(stuff->pixmap, RT_NONE, client->context);
            return rc;
        }
    }

    return Success;
}

static int
ProcCompositeGetOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeGetOverlayWindowReq);
    xCompositeGetOverlayWindowReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    CompScreenPtr cs;
    CompOverlayClientPtr pOc;
    int rc;

    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    pScreen = pWin->drawable.pScreen;

    /*
     * Create an OverlayClient structure to mark this client's
     * interest in the overlay window
     */
    pOc = compCreateOverlayClient(pScreen, client);
    if (pOc == NULL)
        return BadAlloc;

    /*
     * Make sure the overlay window exists
     */
    cs = GetCompScreen(pScreen);
    if (cs->pOverlayWin == NULL)
        if (!compCreateOverlayWindow(pScreen)) {
            FreeResource(pOc->resource, RT_NONE, client->context);
            return BadAlloc;
        }

    rc = XaceHook(XACE_RESOURCE_ACCESS, client, cs->pOverlayWin->drawable.id,
                  RT_WINDOW, cs->pOverlayWin, RT_NONE, NULL, DixGetAttrAccess);
    if (rc != Success) {
        FreeResource(pOc->resource, RT_NONE, client->context);
        return rc;
    }

    rep = (xCompositeGetOverlayWindowReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .overlayWin = cs->pOverlayWin->drawable.id
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.overlayWin);
    }
    WriteToClient(client, sz_xCompositeGetOverlayWindowReply, &rep);

    return Success;
}

static int
ProcCompositeReleaseOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeReleaseOverlayWindowReq);
    WindowPtr pWin;
    CompOverlayClientPtr pOc;

    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);

    /*
     * Has client queried a reference to the overlay window
     * on this screen? If not, generate an error.
     */
    pOc = compFindOverlayClient(pWin->drawable.pScreen, client);
    if (pOc == NULL)
        return BadMatch;

    /* The delete function will free the client structure */
    FreeResource(pOc->resource, RT_NONE, client->context);

    return Success;
}

static int (*ProcCompositeVector[CompositeNumberRequests]) (ClientPtr) = {
ProcCompositeQueryVersion,
        ProcCompositeRedirectWindow,
        ProcCompositeRedirectSubwindows,
        ProcCompositeUnredirectWindow,
        ProcCompositeUnredirectSubwindows,
        ProcCompositeCreateRegionFromBorderClip,
        ProcCompositeNameWindowPixmap,
        ProcCompositeGetOverlayWindow, ProcCompositeReleaseOverlayWindow,};

static int
ProcCompositeDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data < CompositeNumberRequests)
        return (*ProcCompositeVector[stuff->data]) (client);
    else
        return BadRequest;
}

static int _X_COLD
SProcCompositeQueryVersion(ClientPtr client)
{
    REQUEST(xCompositeQueryVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeQueryVersionReq);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeRedirectWindow(ClientPtr client)
{
    REQUEST(xCompositeRedirectWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeRedirectSubwindows(ClientPtr client)
{
    REQUEST(xCompositeRedirectSubwindowsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeUnredirectWindow(ClientPtr client)
{
    REQUEST(xCompositeUnredirectWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeUnredirectSubwindows(ClientPtr client)
{
    REQUEST(xCompositeUnredirectSubwindowsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeCreateRegionFromBorderClip(ClientPtr client)
{
    REQUEST(xCompositeCreateRegionFromBorderClipReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeCreateRegionFromBorderClipReq);
    swapl(&stuff->region);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeNameWindowPixmap(ClientPtr client)
{
    REQUEST(xCompositeNameWindowPixmapReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);
    swapl(&stuff->window);
    swapl(&stuff->pixmap);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeGetOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeGetOverlayWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int _X_COLD
SProcCompositeReleaseOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeReleaseOverlayWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
(*SProcCompositeVector[CompositeNumberRequests]) (ClientPtr) = {
    SProcCompositeQueryVersion,
    SProcCompositeRedirectWindow,
    SProcCompositeRedirectSubwindows,
    SProcCompositeUnredirectWindow,
    SProcCompositeUnredirectSubwindows,
    SProcCompositeCreateRegionFromBorderClip,
    SProcCompositeNameWindowPixmap,
    SProcCompositeGetOverlayWindow,
    SProcCompositeReleaseOverlayWindow,
};

static int _X_COLD
SProcCompositeDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data < CompositeNumberRequests)
        return (*SProcCompositeVector[stuff->data]) (client);
    else
        return BadRequest;
}

/** @see GetDefaultBytes */
static SizeType coreGetWindowBytes;

static void
GetCompositeWindowBytes(void *value, XID id, ResourceSizePtr size)
{
    WindowPtr window = value;

    /* call down */
    coreGetWindowBytes(value, id, size);

    /* account for redirection */
    if (window->redirectDraw != RedirectDrawNone)
    {
        ScreenPtr screen = window->drawable.pScreen;
        XephyrContext* context = screen->context;
        SizeType pixmapSizeFunc = GetResourceTypeSizeFunc(RT_PIXMAP, context);
        ResourceSizeRec pixmapSize = { 0, 0 };
        PixmapPtr pixmap = screen->GetWindowPixmap(window);
        pixmapSizeFunc(pixmap, pixmap->drawable.id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
}

void
CompositeExtensionInit(XephyrContext* context)
{
    ExtensionEntry *extEntry;
    int s;

    /* Assume initialization is going to fail */
    context->noCompositeExtension = TRUE;

    for (s = 0; s < context->screenInfo.numScreens; s++) {
        ScreenPtr pScreen = context->screenInfo.screens[s];
        VisualPtr vis;

        /* Composite on 8bpp pseudocolor root windows appears to fail, so
         * just disable it on anything pseudocolor for safety.
         */
        for (vis = pScreen->visuals; vis->vid != pScreen->rootVisual; vis++);
        if ((vis->class | DynamicClass) == PseudoColor)
            return;

        /* Ensure that Render is initialized, which is required for automatic
         * compositing.
         */
        if (GetPictureScreenIfSet(pScreen) == NULL)
            return;
    }

    context->CompositeClientWindowType = CreateNewResourceType
        (FreeCompositeClientWindow, "CompositeClientWindow", context);
    if (!context->CompositeClientWindowType)
        return;

    coreGetWindowBytes = GetResourceTypeSizeFunc(RT_WINDOW, context);
    SetResourceTypeSizeFunc(RT_WINDOW, GetCompositeWindowBytes, context);

    context->CompositeClientSubwindowsType = CreateNewResourceType
        (FreeCompositeClientSubwindows, "CompositeClientSubwindows", context);
    if (!context->CompositeClientSubwindowsType)
        return;

    context->CompositeClientOverlayType = CreateNewResourceType
        (FreeCompositeClientOverlay, "CompositeClientOverlay", context);
    if (!context->CompositeClientOverlayType)
        return;

    if (!dixRegisterPrivateKey(&context->CompositeClientPrivateKeyRec, PRIVATE_CLIENT,
                               sizeof(CompositeClientRec), context))
        return;

    for (s = 0; s < context->screenInfo.numScreens; s++)
        if (!compScreenInit(context->screenInfo.screens[s]))
            return;

    extEntry = AddExtension(COMPOSITE_NAME, 0, 0,
                            ProcCompositeDispatch, SProcCompositeDispatch,
                            NULL, StandardMinorOpcode, context);
    if (!extEntry)
        return;
    context->CompositeReqCode = (CARD8) extEntry->base;

    /* Initialization succeeded */
    context->noCompositeExtension = FALSE;
}

#ifdef PANORAMIX
#include "panoramiXsrv.h"

/* REMOVED: int (*context->PanoramiXSaveCompositeVector[CompositeNumberRequests]) (ClientPtr); - moved to XephyrContext */

static int
PanoramiXCompositeRedirectWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;

    XephyrContext *context = client->context;
    REQUEST(xCompositeRedirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, context->XRT_WINDOW,
                                      client, DixUnknownAccess, client->context))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*context->PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeRedirectSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;
    XephyrContext *context = client->context;

    REQUEST(xCompositeRedirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, context->XRT_WINDOW,
                                      client, DixUnknownAccess, client->context))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*context->PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeUnredirectWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;
    XephyrContext *context = client->context;

    REQUEST(xCompositeUnredirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, context->XRT_WINDOW,
                                      client, DixUnknownAccess, client->context))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*context->PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeUnredirectSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;
    XephyrContext *context = client->context;

    REQUEST(xCompositeUnredirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, context->XRT_WINDOW,
                                      client, DixUnknownAccess, client->context))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*context->PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeNameWindowPixmap(ClientPtr client)
{
    WindowPtr pWin;

    XephyrContext *context = client->context;    CompWindowPtr cw;
    PixmapPtr pPixmap;
    int rc;
    PanoramiXRes *win, *newPix;
    int i;

    REQUEST(xCompositeNameWindowPixmapReq);

    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, context->XRT_WINDOW,
                                      client, DixUnknownAccess, client->context))) {
        client->errorValue = stuff->window;
        return rc;
    }

    LEGAL_NEW_RESOURCE(stuff->pixmap, client);

    if (!(newPix = malloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newPix->type = context->XRT_PIXMAP;
    newPix->u.pix.shared = FALSE;
    panoramix_setup_ids(newPix, client, stuff->pixmap);

    FOR_NSCREENS(i) {
        rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                     RT_WINDOW, client, DixGetAttrAccess, context);
        if (rc != Success) {
            client->errorValue = stuff->window;
            free(newPix);
            return rc;
        }

        if (!pWin->viewable) {
            free(newPix);
            return BadMatch;
        }

        cw = GetCompWindow(pWin);
        if (!cw) {
            free(newPix);
            return BadMatch;
        }

        pPixmap = (*pWin->drawable.pScreen->GetWindowPixmap) (pWin);
        if (!pPixmap) {
            free(newPix);
            return BadMatch;
        }

        if (!AddResource(newPix->info[i].id, RT_PIXMAP, (void *) pPixmap, client->context))
            return BadAlloc;

        ++pPixmap->refcnt;
    }

    if (!AddResource(stuff->pixmap, context->XRT_PIXMAP, (void *) newPix, client->context))
        return BadAlloc;

    return Success;
}

static int
PanoramiXCompositeGetOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeGetOverlayWindowReq);

    XephyrContext *context = client->context;    xCompositeGetOverlayWindowReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    CompScreenPtr cs;
    CompOverlayClientPtr pOc;
    int rc;
    PanoramiXRes *win, *overlayWin = NULL;
    int i;

    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, context->XRT_WINDOW,
                                      client, DixUnknownAccess, client->context))) {
        client->errorValue = stuff->window;
        return rc;
    }

    cs = GetCompScreen(client->context->screenInfo.screens[0]);
    if (!cs->pOverlayWin) {
        if (!(overlayWin = malloc(sizeof(PanoramiXRes))))
            return BadAlloc;

        overlayWin->type = context->XRT_WINDOW;
        overlayWin->u.win.root = FALSE;
    }

    FOR_NSCREENS_BACKWARD(i) {
        rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                     RT_WINDOW, client, DixGetAttrAccess, context);
        if (rc != Success) {
            client->errorValue = stuff->window;
            free(overlayWin);
            return rc;
        }
        pScreen = pWin->drawable.pScreen;

        /*
         * Create an OverlayClient structure to mark this client's
         * interest in the overlay window
         */
        pOc = compCreateOverlayClient(pScreen, client);
        if (pOc == NULL) {
            free(overlayWin);
            return BadAlloc;
        }

        /*
         * Make sure the overlay window exists
         */
        cs = GetCompScreen(pScreen);
        if (cs->pOverlayWin == NULL)
            if (!compCreateOverlayWindow(pScreen)) {
                FreeResource(pOc->resource, RT_NONE, client->context);
                free(overlayWin);
                return BadAlloc;
            }

        rc = XaceHook(XACE_RESOURCE_ACCESS, client,
                      cs->pOverlayWin->drawable.id,
                      RT_WINDOW, cs->pOverlayWin, RT_NONE, NULL,
                      DixGetAttrAccess);
        if (rc != Success) {
            FreeResource(pOc->resource, RT_NONE, client->context);
            free(overlayWin);
            return rc;
        }
    }

    if (overlayWin) {
        FOR_NSCREENS(i) {
            cs = GetCompScreen(client->context->screenInfo.screens[i]);
            overlayWin->info[i].id = cs->pOverlayWin->drawable.id;
        }

        AddResource(overlayWin->info[0].id, context->XRT_WINDOW, overlayWin, client->context);
    }

    cs = GetCompScreen(client->context->screenInfo.screens[0]);

    rep = (xCompositeGetOverlayWindowReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .overlayWin = cs->pOverlayWin->drawable.id
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.overlayWin);
    }
    WriteToClient(client, sz_xCompositeGetOverlayWindowReply, &rep);

    return Success;
}

static int
PanoramiXCompositeReleaseOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeReleaseOverlayWindowReq);
    WindowPtr pWin;
    CompOverlayClientPtr pOc;
    PanoramiXRes *win;
    int i, rc;
    XephyrContext *context = client->context;

    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, context->XRT_WINDOW,
                                      client, DixUnknownAccess, client->context))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_BACKWARD(i) {
        if ((rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                          context->XRT_WINDOW, client,
                                          DixUnknownAccess, client->context))) {
            client->errorValue = stuff->window;
            return rc;
        }

        /*
         * Has client queried a reference to the overlay window
         * on this screen? If not, generate an error.
         */
        pOc = compFindOverlayClient(pWin->drawable.pScreen, client);
        if (pOc == NULL)
            return BadMatch;

        /* The delete function will free the client structure */
        FreeResource(pOc->resource, RT_NONE, client->context);
    }

    return Success;
}

void
PanoramiXCompositeInit(XephyrContext* context)
{
    int i;

    for (i = 0; i < CompositeNumberRequests; i++)
        context->PanoramiXSaveCompositeVector[i] = ProcCompositeVector[i];
    /*
     * Stuff in Xinerama aware request processing hooks
     */
    ProcCompositeVector[X_CompositeRedirectWindow] =
        PanoramiXCompositeRedirectWindow;
    ProcCompositeVector[X_CompositeRedirectSubwindows] =
        PanoramiXCompositeRedirectSubwindows;
    ProcCompositeVector[X_CompositeUnredirectWindow] =
        PanoramiXCompositeUnredirectWindow;
    ProcCompositeVector[X_CompositeUnredirectSubwindows] =
        PanoramiXCompositeUnredirectSubwindows;
    ProcCompositeVector[X_CompositeNameWindowPixmap] =
        PanoramiXCompositeNameWindowPixmap;
    ProcCompositeVector[X_CompositeGetOverlayWindow] =
        PanoramiXCompositeGetOverlayWindow;
    ProcCompositeVector[X_CompositeReleaseOverlayWindow] =
        PanoramiXCompositeReleaseOverlayWindow;
}

void
PanoramiXCompositeReset(XephyrContext* context)
{
    int i;

    for (i = 0; i < CompositeNumberRequests; i++)
        ProcCompositeVector[i] = context->PanoramiXSaveCompositeVector[i];
}

#endif
