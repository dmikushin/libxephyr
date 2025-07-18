#include "dix/context.h"
/***********************************************************
Copyright 1991 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or MIT not be
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
** File:
**
**   xvmain.c --- Xv server extension main device independent module.
**
** Author:
**
**   David Carver (Digital Workstation Engineering/Project Athena)
**
** Revisions:
**
**   04.09.91 Carver
**     - change: stop video always generates an event even when video
**       wasn't active
**
**   29.08.91 Carver
**     - change: unrealizing windows no longer preempts video
**
**   11.06.91 Carver
**     - changed SetPortControl to SetPortAttribute
**     - changed GetPortControl to GetPortAttribute
**     - changed QueryBestSize
**
**   28.05.91 Carver
**     - fixed Put and Get requests to not preempt operations to same drawable
**
**   15.05.91 Carver
**     - version 2.0 upgrade
**
**   19.03.91 Carver
**     - fixed Put and Get requests to honor grabbed ports.
**     - fixed Video requests to update di structure with new drawable, and
**       client after calling ddx.
**
**   24.01.91 Carver
**     - version 1.4 upgrade
**
** Notes:
**
**   Port structures reference client structures in a two different
**   ways: when grabs, or video is active.  Each reference is encoded
**   as fake client resources and thus when the client is goes away so
**   does the reference (it is zeroed).  No other action is taken, so
**   video doesn't necessarily stop.  It probably will as a result of
**   other resources going away, but if a client starts video using
**   none of its own resources, then the video will continue to play
**   after the client disappears.
**
**
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "extnsionst.h"
#include "extinit.h"
#include "dixstruct.h"
#include "resource.h"
#include "opaque.h"
#include "input.h"

#define GLOBAL

#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvproto.h>
#include "xvdix.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif
#include "xvdisp.h"

static DevPrivateKeyRec XvScreenKeyRec;

#define XvScreenKey (&XvScreenKeyRec)
// Global variables moved to XephyrContext:
// XvExtensionGeneration, XvScreenGeneration, XvResourceGeneration
// XvReqCode, XvEventBase, XvErrorBase

// Globals moved to XephyrContext:
// RESTYPE XvRTPort;
// RESTYPE XvRTEncoding;
// RESTYPE XvRTGrab;
// RESTYPE XvRTVideoNotify;
// RESTYPE XvRTVideoNotifyList;
// RESTYPE XvRTPortNotify;

/* EXTERNAL */

static void WriteSwappedVideoNotifyEvent(xvEvent *, xvEvent *);
static void WriteSwappedPortNotifyEvent(xvEvent *, xvEvent *);
static Bool CreateResourceTypes(XephyrContext* context);

static Bool XvCloseScreen(ScreenPtr);
static Bool XvDestroyPixmap(PixmapPtr);
static Bool XvDestroyWindow(WindowPtr);
static void XvResetProc(ExtensionEntry *);
static int XvdiDestroyGrab(void *, XID, XephyrContext*);
static int XvdiDestroyEncoding(void *, XID, XephyrContext*);
static int XvdiDestroyVideoNotify(void *, XID, XephyrContext*);
static int XvdiDestroyPortNotify(void *, XID, XephyrContext*);
static int XvdiDestroyVideoNotifyList(void *, XID, XephyrContext*);
static int XvdiDestroyPort(void *, XID, XephyrContext*);
static int XvdiSendVideoNotify(XvPortPtr, DrawablePtr, int, XephyrContext*);

/*
** XvExtensionInit
**
**
*/

void
XvExtensionInit(XephyrContext* context)
{
    ExtensionEntry *extEntry;

    if (!dixRegisterPrivateKey(&XvScreenKeyRec, PRIVATE_SCREEN, 0, context))
        return;

    /* Look to see if any screens were initialized; if not then
       init global variables so the extension can function */
    if (context->XvScreenGeneration != context->serverGeneration) {
        if (!CreateResourceTypes(context)) {
            ErrorF("XvExtensionInit: Unable to allocate resource types\n", context);
            return;
        }
#ifdef PANORAMIX
        XineramaRegisterConnectionBlockCallback(XineramifyXv, context);
#endif
        context->XvScreenGeneration = context->serverGeneration;
    }

    if (context->XvExtensionGeneration != context->serverGeneration) {
        context->XvExtensionGeneration = context->serverGeneration;

        extEntry = AddExtension(XvName, XvNumEvents, XvNumErrors,
                                ProcXvDispatch, SProcXvDispatch,
                                XvResetProc, StandardMinorOpcode, context);
        if (!extEntry) {
            FatalError("XvExtensionInit: AddExtensions failed\n", context);
        }

        context->XvReqCode = extEntry->base;
        context->XvEventBase = extEntry->eventBase;
        context->XvErrorBase = extEntry->errorBase;

        EventSwapVector[context->XvEventBase + XvVideoNotify] =
            (EventSwapPtr) WriteSwappedVideoNotifyEvent;
        EventSwapVector[context->XvEventBase + XvPortNotify] =
            (EventSwapPtr) WriteSwappedPortNotifyEvent;

        SetResourceTypeErrorValue(context->XvRTPort, XvBadPort + context->XvErrorBase, context);
        (void) MakeAtom(XvName, strlen(XvName), xTrue);

    }
}

static Bool
CreateResourceTypes(XephyrContext* context)
{

    if (context->XvResourceGeneration == context->serverGeneration)
        return TRUE;

    context->XvResourceGeneration = context->serverGeneration;

    if (!(context->XvRTPort = CreateNewResourceType(XvdiDestroyPort, "context->XvRTPort", context))) {
        ErrorF("CreateResourceTypes: failed to allocate port resource.\n", context);
        return FALSE;
    }

    if (!(context->XvRTGrab = CreateNewResourceType(XvdiDestroyGrab, "context->XvRTGrab", context))) {
        ErrorF("CreateResourceTypes: failed to allocate grab resource.\n", context);
        return FALSE;
    }

    if (!(context->XvRTEncoding = CreateNewResourceType(XvdiDestroyEncoding,
                                               "context->XvRTEncoding", context))) {
        ErrorF("CreateResourceTypes: failed to allocate encoding resource.\n", context);
        return FALSE;
    }

    if (!(context->XvRTVideoNotify = CreateNewResourceType(XvdiDestroyVideoNotify,
                                                  "context->XvRTVideoNotify", context))) {
        ErrorF
            ("CreateResourceTypes: failed to allocate video notify resource.\n", context);
        return FALSE;
    }

    if (!
        (context->XvRTVideoNotifyList =
         CreateNewResourceType(XvdiDestroyVideoNotifyList,
                               "context->XvRTVideoNotifyList", context))) {
        ErrorF
            ("CreateResourceTypes: failed to allocate video notify list resource.\n", context);
        return FALSE;
    }

    if (!(context->XvRTPortNotify = CreateNewResourceType(XvdiDestroyPortNotify,
                                                 "context->XvRTPortNotify", context))) {
        ErrorF
            ("CreateResourceTypes: failed to allocate port notify resource.\n", context);
        return FALSE;
    }

    return TRUE;

}

int
XvScreenInit(ScreenPtr pScreen, XephyrContext* context)
{
    XvScreenPtr pxvs;

    if (context->XvScreenGeneration != context->serverGeneration) {
        if (!CreateResourceTypes(context)) {
            ErrorF("XvScreenInit: Unable to allocate resource types\n", context);
            return BadAlloc;
        }
#ifdef PANORAMIX
        XineramaRegisterConnectionBlockCallback(XineramifyXv, context);
#endif
        context->XvScreenGeneration = context->serverGeneration;
    }

    if (!dixRegisterPrivateKey(&XvScreenKeyRec, PRIVATE_SCREEN, 0, context))
        return BadAlloc;

    if (dixLookupPrivate(&pScreen->devPrivates, XvScreenKey)) {
        ErrorF("XvScreenInit: screen devPrivates ptr non-NULL before init\n", context);
    }

    /* ALLOCATE SCREEN PRIVATE RECORD */

    pxvs = malloc(sizeof(XvScreenRec));
    if (!pxvs) {
        ErrorF("XvScreenInit: Unable to allocate screen private structure\n", context);
        return BadAlloc;
    }

    dixSetPrivate(&pScreen->devPrivates, XvScreenKey, pxvs);

    pxvs->DestroyPixmap = pScreen->DestroyPixmap;
    pxvs->DestroyWindow = pScreen->DestroyWindow;
    pxvs->CloseScreen = pScreen->CloseScreen;

    pScreen->DestroyPixmap = XvDestroyPixmap;
    pScreen->DestroyWindow = XvDestroyWindow;
    pScreen->CloseScreen = XvCloseScreen;

    return Success;
}

static Bool
XvCloseScreen(ScreenPtr pScreen)
{

    XvScreenPtr pxvs;

    pxvs = (XvScreenPtr) dixLookupPrivate(&pScreen->devPrivates, XvScreenKey);

    pScreen->DestroyPixmap = pxvs->DestroyPixmap;
    pScreen->DestroyWindow = pxvs->DestroyWindow;
    pScreen->CloseScreen = pxvs->CloseScreen;

    free(pxvs);

    dixSetPrivate(&pScreen->devPrivates, XvScreenKey, NULL);

    return (*pScreen->CloseScreen) (pScreen);
}

static void
XvResetProc(ExtensionEntry * extEntry)
{
    XvResetProcVector();
}

DevPrivateKey
XvGetScreenKey(void)
{
    return XvScreenKey;
}

unsigned long
XvGetRTPort(XephyrContext* context)
{
    return context->XvRTPort;
}

static void
XvStopAdaptors(DrawablePtr pDrawable, XephyrContext* context)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    XvScreenPtr pxvs = dixLookupPrivate(&pScreen->devPrivates, XvScreenKey);
    XvAdaptorPtr pa = pxvs->pAdaptors;
    int na = pxvs->nAdaptors;

    /* CHECK TO SEE IF THIS PORT IS IN USE */
    while (na--) {
        XvPortPtr pp = pa->pPorts;
        int np = pa->nPorts;

        while (np--) {
            if (pp->pDraw == pDrawable) {
                XvdiSendVideoNotify(pp, pDrawable, XvPreempted, context);

                (void) (*pp->pAdaptor->ddStopVideo) (pp, pDrawable);

                pp->pDraw = NULL;
                pp->client = NULL;
                pp->time = context->currentTime;
            }
            pp++;
        }
        pa++;
    }
}

static Bool
XvDestroyPixmap(PixmapPtr pPix)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    Bool status;

    if (pPix->refcnt == 1)
        XvStopAdaptors(&pPix->drawable, NULL);

    SCREEN_PROLOGUE(pScreen, DestroyPixmap);
    status = (*pScreen->DestroyPixmap) (pPix);
    SCREEN_EPILOGUE(pScreen, DestroyPixmap, XvDestroyPixmap);

    return status;

}

static Bool
XvDestroyWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    Bool status;

    XvStopAdaptors(&pWin->drawable, NULL);

    SCREEN_PROLOGUE(pScreen, DestroyWindow);
    status = (*pScreen->DestroyWindow) (pWin);
    SCREEN_EPILOGUE(pScreen, DestroyWindow, XvDestroyWindow);

    return status;

}

static int
XvdiDestroyPort(void *pPort, XID id, XephyrContext* context)
{
    return Success;
}

static int
XvdiDestroyGrab(void *pGrab, XID id, XephyrContext* context)
{
    ((XvGrabPtr) pGrab)->client = NULL;
    return Success;
}

static int
XvdiDestroyVideoNotify(void *pn, XID id, XephyrContext* context)
{
    /* JUST CLEAR OUT THE client POINTER FIELD */

    ((XvVideoNotifyPtr) pn)->client = NULL;
    return Success;
}

static int
XvdiDestroyPortNotify(void *pn, XID id, XephyrContext* context)
{
    /* JUST CLEAR OUT THE client POINTER FIELD */

    ((XvPortNotifyPtr) pn)->client = NULL;
    return Success;
}

static int
XvdiDestroyVideoNotifyList(void *pn, XID id, XephyrContext* context)
{
    XvVideoNotifyPtr npn, cpn;

    /* ACTUALLY DESTROY THE NOTIFY LIST */

    cpn = (XvVideoNotifyPtr) pn;

    while (cpn) {
        npn = cpn->next;
        if (cpn->client)
            FreeResource(cpn->id, context->XvRTVideoNotify, context);
        free(cpn);
        cpn = npn;
    }
    return Success;
}

static int
XvdiDestroyEncoding(void *value, XID id, XephyrContext* context)
{
    return Success;
}

static int
XvdiSendVideoNotify(XvPortPtr pPort, DrawablePtr pDraw, int reason, XephyrContext* context)
{
    XvVideoNotifyPtr pn;

    dixLookupResourceByType((void **) &pn, pDraw->id, context->XvRTVideoNotifyList,
                            context->serverClient, DixReadAccess, context);

    while (pn) {
        xvEvent event = {
            .u.videoNotify.reason = reason,
            .u.videoNotify.time = context->currentTime.milliseconds,
            .u.videoNotify.drawable = pDraw->id,
            .u.videoNotify.port = pPort->id
        };
        event.u.u.type = context->XvEventBase + XvVideoNotify;
        WriteEventsToClient(pn->client, 1, (xEventPtr) &event);
        pn = pn->next;
    }

    return Success;

}

int
XvdiSendPortNotify(XvPortPtr pPort, Atom attribute, INT32 value, XephyrContext* context)
{
    XvPortNotifyPtr pn;

    pn = pPort->pNotify;

    while (pn) {
        xvEvent event = {
            .u.portNotify.time = context->currentTime.milliseconds,
            .u.portNotify.port = pPort->id,
            .u.portNotify.attribute = attribute,
            .u.portNotify.value = value
        };
        event.u.u.type = context->XvEventBase + XvPortNotify;
        WriteEventsToClient(pn->client, 1, (xEventPtr) &event);
        pn = pn->next;
    }

    return Success;

}

#define CHECK_SIZE(dw, dh, sw, sh) {                                  \
  if(!dw || !dh || !sw || !sh)  return Success;                       \
  /* The region code will break these if they are too large */        \
  if((dw > 32767) || (dh > 32767) || (sw > 32767) || (sh > 32767))    \
        return BadValue;                                              \
}

int
XvdiPutVideo(ClientPtr client,
             DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    DrawablePtr pOldDraw;

    CHECK_SIZE(drw_w, drw_h, vid_w, vid_h);

    /* UPDATE TIME VARIABLES FOR USE IN EVENTS */

    UpdateCurrentTime(client->context);

    /* CHECK FOR GRAB; IF THIS CLIENT DOESN'T HAVE THE PORT GRABBED THEN
       INFORM CLIENT OF ITS FAILURE */

    if (pPort->grab.client && (pPort->grab.client != client)) {
        XvdiSendVideoNotify(pPort, pDraw, XvBusy, client->context);
        return Success;
    }

    /* CHECK TO SEE IF PORT IS IN USE; IF SO THEN WE MUST DELIVER INTERRUPTED
       EVENTS TO ANY CLIENTS WHO WANT THEM */

    pOldDraw = pPort->pDraw;
    if ((pOldDraw) && (pOldDraw != pDraw)) {
        XvdiSendVideoNotify(pPort, pPort->pDraw, XvPreempted, client->context);
    }

    (void) (*pPort->pAdaptor->ddPutVideo) (pDraw, pPort, pGC,
                                           vid_x, vid_y, vid_w, vid_h,
                                           drw_x, drw_y, drw_w, drw_h);

    if ((pPort->pDraw) && (pOldDraw != pDraw)) {
        pPort->client = client;
        XvdiSendVideoNotify(pPort, pPort->pDraw, XvStarted, client->context);
    }

    pPort->time = client->context->currentTime;

    return Success;

}

int
XvdiPutStill(ClientPtr client,
             DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    int status;

    CHECK_SIZE(drw_w, drw_h, vid_w, vid_h);

    /* UPDATE TIME VARIABLES FOR USE IN EVENTS */

    UpdateCurrentTime(client->context);

    /* CHECK FOR GRAB; IF THIS CLIENT DOESN'T HAVE THE PORT GRABBED THEN
       INFORM CLIENT OF ITS FAILURE */

    if (pPort->grab.client && (pPort->grab.client != client)) {
        XvdiSendVideoNotify(pPort, pDraw, XvBusy, client->context);
        return Success;
    }

    pPort->time = client->context->currentTime;

    status = (*pPort->pAdaptor->ddPutStill) (pDraw, pPort, pGC,
                                             vid_x, vid_y, vid_w, vid_h,
                                             drw_x, drw_y, drw_w, drw_h);

    return status;

}

int
XvdiPutImage(ClientPtr client,
             DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 src_x, INT16 src_y,
             CARD16 src_w, CARD16 src_h,
             INT16 drw_x, INT16 drw_y,
             CARD16 drw_w, CARD16 drw_h,
             XvImagePtr image,
             unsigned char *data, Bool sync, CARD16 width, CARD16 height)
{
    CHECK_SIZE(drw_w, drw_h, src_w, src_h);

    /* UPDATE TIME VARIABLES FOR USE IN EVENTS */

    UpdateCurrentTime(client->context);

    /* CHECK FOR GRAB; IF THIS CLIENT DOESN'T HAVE THE PORT GRABBED THEN
       INFORM CLIENT OF ITS FAILURE */

    if (pPort->grab.client && (pPort->grab.client != client)) {
        XvdiSendVideoNotify(pPort, pDraw, XvBusy, client->context);
        return Success;
    }

    pPort->time = client->context->currentTime;

    return (*pPort->pAdaptor->ddPutImage) (pDraw, pPort, pGC,
                                           src_x, src_y, src_w, src_h,
                                           drw_x, drw_y, drw_w, drw_h,
                                           image, data, sync, width, height);
}

int
XvdiGetVideo(ClientPtr client,
             DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    DrawablePtr pOldDraw;

    CHECK_SIZE(drw_w, drw_h, vid_w, vid_h);

    /* UPDATE TIME VARIABLES FOR USE IN EVENTS */

    UpdateCurrentTime(client->context);

    /* CHECK FOR GRAB; IF THIS CLIENT DOESN'T HAVE THE PORT GRABBED THEN
       INFORM CLIENT OF ITS FAILURE */

    if (pPort->grab.client && (pPort->grab.client != client)) {
        XvdiSendVideoNotify(pPort, pDraw, XvBusy, client->context);
        return Success;
    }

    /* CHECK TO SEE IF PORT IS IN USE; IF SO THEN WE MUST DELIVER INTERRUPTED
       EVENTS TO ANY CLIENTS WHO WANT THEM */

    pOldDraw = pPort->pDraw;
    if ((pOldDraw) && (pOldDraw != pDraw)) {
        XvdiSendVideoNotify(pPort, pPort->pDraw, XvPreempted, client->context);
    }

    (void) (*pPort->pAdaptor->ddGetVideo) (pDraw, pPort, pGC,
                                           vid_x, vid_y, vid_w, vid_h,
                                           drw_x, drw_y, drw_w, drw_h);

    if ((pPort->pDraw) && (pOldDraw != pDraw)) {
        pPort->client = client;
        XvdiSendVideoNotify(pPort, pPort->pDraw, XvStarted, client->context);
    }

    pPort->time = client->context->currentTime;

    return Success;

}

int
XvdiGetStill(ClientPtr client,
             DrawablePtr pDraw,
             XvPortPtr pPort,
             GCPtr pGC,
             INT16 vid_x, INT16 vid_y,
             CARD16 vid_w, CARD16 vid_h,
             INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    int status;

    CHECK_SIZE(drw_w, drw_h, vid_w, vid_h);

    /* UPDATE TIME VARIABLES FOR USE IN EVENTS */

    UpdateCurrentTime(client->context);

    /* CHECK FOR GRAB; IF THIS CLIENT DOESN'T HAVE THE PORT GRABBED THEN
       INFORM CLIENT OF ITS FAILURE */

    if (pPort->grab.client && (pPort->grab.client != client)) {
        XvdiSendVideoNotify(pPort, pDraw, XvBusy, client->context);
        return Success;
    }

    status = (*pPort->pAdaptor->ddGetStill) (pDraw, pPort, pGC,
                                             vid_x, vid_y, vid_w, vid_h,
                                             drw_x, drw_y, drw_w, drw_h);

    pPort->time = client->context->currentTime;

    return status;

}

int
XvdiGrabPort(ClientPtr client, XvPortPtr pPort, Time ctime, int *p_result)
{
    unsigned long id;
    TimeStamp time;

    UpdateCurrentTime(client->context);
    time = ClientTimeToServerTime(ctime, client->context);

    if (pPort->grab.client && (client != pPort->grab.client)) {
        *p_result = XvAlreadyGrabbed;
        return Success;
    }

    if ((CompareTimeStamps(time, client->context->currentTime) == LATER) ||
        (CompareTimeStamps(time, pPort->time) == EARLIER)) {
        *p_result = XvInvalidTime;
        return Success;
    }

    if (client == pPort->grab.client) {
        *p_result = Success;
        return Success;
    }

    id = FakeClientID(client->index, client->context);

    if (!AddResource(id, client->context->XvRTGrab, &pPort->grab, client->context)) {
        return BadAlloc;
    }

    /* IF THERE IS ACTIVE VIDEO THEN STOP IT */

    if ((pPort->pDraw) && (client != pPort->client)) {
        XvdiStopVideo(NULL, pPort, pPort->pDraw);
    }

    pPort->grab.client = client;
    pPort->grab.id = id;

    pPort->time = client->context->currentTime;

    *p_result = Success;

    return Success;

}

int
XvdiUngrabPort(ClientPtr client, XvPortPtr pPort, Time ctime)
{
    TimeStamp time;
    XephyrContext* context = client->context;

    UpdateCurrentTime(client->context);
    time = ClientTimeToServerTime(ctime, client->context);

    if ((!pPort->grab.client) || (client != pPort->grab.client)) {
        return Success;
    }

    if ((CompareTimeStamps(time, client->context->currentTime) == LATER) ||
        (CompareTimeStamps(time, pPort->time) == EARLIER)) {
        return Success;
    }

    /* FREE THE GRAB RESOURCE; AND SET THE GRAB CLIENT TO NULL */

    FreeResource(pPort->grab.id, client->context->XvRTGrab, client->context);
    pPort->grab.client = NULL;

    pPort->time = client->context->currentTime;

    return Success;

}

int
XvdiSelectVideoNotify(ClientPtr client, DrawablePtr pDraw, BOOL onoff)
{
    XvVideoNotifyPtr pn, tpn, fpn;
    int rc;

    /* FIND VideoNotify LIST */

    rc = dixLookupResourceByType((void **) &pn, pDraw->id,
                                 client->context->XvRTVideoNotifyList, client, DixWriteAccess, client->context);
    if (rc != Success && rc != BadValue)
        return rc;

    /* IF ONE DONES'T EXIST AND NO MASK, THEN JUST RETURN */

    if (!onoff && !pn)
        return Success;

    /* IF ONE DOESN'T EXIST CREATE IT AND ADD A RESOURCE SO THAT THE LIST
       WILL BE DELETED WHEN THE DRAWABLE IS DESTROYED */

    if (!pn) {
        if (!(tpn = malloc(sizeof(XvVideoNotifyRec))))
            return BadAlloc;
        tpn->next = NULL;
        tpn->client = NULL;
        if (!AddResource(pDraw->id, client->context->XvRTVideoNotifyList, tpn, client->context))
            return BadAlloc;
    }
    else {
        /* LOOK TO SEE IF ENTRY ALREADY EXISTS */

        fpn = NULL;
        tpn = pn;
        while (tpn) {
            if (tpn->client == client) {
                if (!onoff)
                    tpn->client = NULL;
                return Success;
            }
            if (!tpn->client)
                fpn = tpn;      /* TAKE NOTE OF FREE ENTRY */
            tpn = tpn->next;
        }

        /* IF TURNING OFF, THEN JUST RETURN */

        if (!onoff)
            return Success;

        /* IF ONE ISN'T FOUND THEN ALLOCATE ONE AND LINK IT INTO THE LIST */

        if (fpn) {
            tpn = fpn;
        }
        else {
            if (!(tpn = malloc(sizeof(XvVideoNotifyRec))))
                return BadAlloc;
            tpn->next = pn->next;
            pn->next = tpn;
        }
    }

    /* INIT CLIENT PTR IN CASE WE CAN'T ADD RESOURCE */
    /* ADD RESOURCE SO THAT IF CLIENT EXITS THE CLIENT PTR WILL BE CLEARED */

    tpn->client = NULL;
    tpn->id = FakeClientID(client->index, client->context);
    if (!AddResource(tpn->id, client->context->XvRTVideoNotify, tpn, client->context))
        return BadAlloc;

    tpn->client = client;
    return Success;

}

int
XvdiSelectPortNotify(ClientPtr client, XvPortPtr pPort, BOOL onoff)
{
    XvPortNotifyPtr pn, tpn;

    /* SEE IF CLIENT IS ALREADY IN LIST */

    tpn = NULL;
    pn = pPort->pNotify;
    while (pn) {
        if (!pn->client)
            tpn = pn;           /* TAKE NOTE OF FREE ENTRY */
        if (pn->client == client)
            break;
        pn = pn->next;
    }

    /* IS THE CLIENT ALREADY ON THE LIST? */

    if (pn) {
        /* REMOVE IT? */

        if (!onoff) {
            pn->client = NULL;
            FreeResource(pn->id, client->context->XvRTPortNotify, client->context);
        }

        return Success;
    }

    /* DIDN'T FIND IT; SO REUSE LIST ELEMENT IF ONE IS FREE OTHERWISE
       CREATE A NEW ONE AND ADD IT TO THE BEGINNING OF THE LIST */

    if (!tpn) {
        if (!(tpn = malloc(sizeof(XvPortNotifyRec))))
            return BadAlloc;
        tpn->next = pPort->pNotify;
        pPort->pNotify = tpn;
    }

    tpn->client = client;
    tpn->id = FakeClientID(client->index, client->context);
    if (!AddResource(tpn->id, client->context->XvRTPortNotify, tpn, client->context))
        return BadAlloc;

    return Success;

}

int
XvdiStopVideo(ClientPtr client, XvPortPtr pPort, DrawablePtr pDraw)
{
    int status;
    XephyrContext* context = client ? client->context : NULL;

    /* IF PORT ISN'T ACTIVE THEN WE'RE DONE */

    if (!pPort->pDraw || (pPort->pDraw != pDraw)) {
        XvdiSendVideoNotify(pPort, pDraw, XvStopped, context);
        return Success;
    }

    /* CHECK FOR GRAB; IF THIS CLIENT DOESN'T HAVE THE PORT GRABBED THEN
       INFORM CLIENT OF ITS FAILURE */

    if ((client) && (pPort->grab.client) && (pPort->grab.client != client)) {
        XvdiSendVideoNotify(pPort, pDraw, XvBusy, client->context);
        return Success;
    }

    XvdiSendVideoNotify(pPort, pDraw, XvStopped, context);

    status = (*pPort->pAdaptor->ddStopVideo) (pPort, pDraw);

    pPort->pDraw = NULL;
    pPort->client = (ClientPtr) client;
    if (context)
        pPort->time = client->context->currentTime;

    return status;

}

int
XvdiMatchPort(XvPortPtr pPort, DrawablePtr pDraw)
{

    XvAdaptorPtr pa;
    XvFormatPtr pf;
    int nf;

    pa = pPort->pAdaptor;

    if (pa->pScreen != pDraw->pScreen)
        return BadMatch;

    nf = pa->nFormats;
    pf = pa->pFormats;

    while (nf--) {
        if (pf->depth == pDraw->depth)
            return Success;
        pf++;
    }

    return BadMatch;

}

int
XvdiSetPortAttribute(ClientPtr client,
                     XvPortPtr pPort, Atom attribute, INT32 value)
{
    int status;

    status =
        (*pPort->pAdaptor->ddSetPortAttribute) (pPort, attribute,
                                                value);
    if (status == Success)
        XvdiSendPortNotify(pPort, attribute, value, client->context);

    return status;
}

int
XvdiGetPortAttribute(ClientPtr client,
                     XvPortPtr pPort, Atom attribute, INT32 *p_value)
{

    return
        (*pPort->pAdaptor->ddGetPortAttribute) (pPort, attribute,
                                                p_value);

}

static void _X_COLD
WriteSwappedVideoNotifyEvent(xvEvent * from, xvEvent * to)
{

    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;
    cpswaps(from->u.videoNotify.sequenceNumber,
            to->u.videoNotify.sequenceNumber);
    cpswapl(from->u.videoNotify.time, to->u.videoNotify.time);
    cpswapl(from->u.videoNotify.drawable, to->u.videoNotify.drawable);
    cpswapl(from->u.videoNotify.port, to->u.videoNotify.port);

}

static void _X_COLD
WriteSwappedPortNotifyEvent(xvEvent * from, xvEvent * to)
{

    to->u.u.type = from->u.u.type;
    to->u.u.detail = from->u.u.detail;
    cpswaps(from->u.portNotify.sequenceNumber, to->u.portNotify.sequenceNumber);
    cpswapl(from->u.portNotify.time, to->u.portNotify.time);
    cpswapl(from->u.portNotify.port, to->u.portNotify.port);
    cpswapl(from->u.portNotify.value, to->u.portNotify.value);

}

void
XvFreeAdaptor(XvAdaptorPtr pAdaptor)
{
    int i;

    free(pAdaptor->name);
    pAdaptor->name = NULL;

    if (pAdaptor->pEncodings) {
        XvEncodingPtr pEncode = pAdaptor->pEncodings;

        for (i = 0; i < pAdaptor->nEncodings; i++, pEncode++)
            free(pEncode->name);
        free(pAdaptor->pEncodings);
        pAdaptor->pEncodings = NULL;
    }

    free(pAdaptor->pFormats);
    pAdaptor->pFormats = NULL;

    free(pAdaptor->pPorts);
    pAdaptor->pPorts = NULL;

    if (pAdaptor->pAttributes) {
        XvAttributePtr pAttribute = pAdaptor->pAttributes;

        for (i = 0; i < pAdaptor->nAttributes; i++, pAttribute++)
            free(pAttribute->name);
        free(pAdaptor->pAttributes);
        pAdaptor->pAttributes = NULL;
    }

    free(pAdaptor->pImages);
    pAdaptor->pImages = NULL;

    free(pAdaptor->devPriv.ptr);
    pAdaptor->devPriv.ptr = NULL;
}

void
XvFillColorKey(DrawablePtr pDraw, CARD32 key, RegionPtr region)
{
    ScreenPtr pScreen = pDraw->pScreen;
    ChangeGCVal pval[2];
    BoxPtr pbox = RegionRects(region);
    int i, nbox = RegionNumRects(region);
    xRectangle *rects;
    GCPtr gc;

    gc = GetScratchGC(pDraw->depth, pScreen);
    if (!gc)
        return;

    pval[0].val = key;
    pval[1].val = IncludeInferiors;
    (void) ChangeGC(NullClient, gc, GCForeground | GCSubwindowMode, pval);
    ValidateGC(pDraw, gc);

    rects = xallocarray(nbox, sizeof(xRectangle));
    if (rects) {
        for (i = 0; i < nbox; i++, pbox++) {
            rects[i].x = pbox->x1 - pDraw->x;
            rects[i].y = pbox->y1 - pDraw->y;
            rects[i].width = pbox->x2 - pbox->x1;
            rects[i].height = pbox->y2 - pbox->y1;
        }

        (*gc->ops->PolyFillRect) (pDraw, gc, nbox, rects);

        free(rects);
    }
    FreeScratchGC(gc);
}
