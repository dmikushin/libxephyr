#include "dix/context.h"
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

#ifdef HAVE_DMX_CONFIG_H
#include <dmx-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xarch.h>
#include "misc.h"
#include "cursor.h"
#include "cursorstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "panoramiX.h"
#include <X11/extensions/panoramiXproto.h>
#include "panoramiXsrv.h"
#include "globals.h"
#include "servermd.h"
#include "resource.h"
#include "picturestr.h"
#include "xfixesint.h"
#include "damageextint.h"
#ifdef COMPOSITE
#include "compint.h"
#endif
#include "extinit.h"
#include "protocol-versions.h"


#ifdef GLXPROXY
extern VisualPtr glxMatchVisual(ScreenPtr pScreen,
                                VisualPtr pVisual, ScreenPtr pMatchScreen);
#endif

/*
 *	PanoramiX data declarations
 */

/* REMOVED: int context->PanoramiXPixWidth = 0; - moved to XephyrContext */
/* REMOVED: int context->PanoramiXPixHeight = 0; - moved to XephyrContext */
/* REMOVED: int context->PanoramiXNumScreens = 0; - moved to XephyrContext */

/* REMOVED: RegionRec context->PanoramiXScreenRegion - moved to XephyrContext */

static int PanoramiXNumDepths;
static DepthPtr PanoramiXDepths;
// REMOVED: static int context->PanoramiXNumVisuals; - moved to XephyrContext
// REMOVED: static VisualPtr context->PanoramiXVisuals; - moved to XephyrContext

/* REMOVED: RESTYPE XRC_DRAWABLE; - moved to XephyrContext */
/* RESTYPE XRT_WINDOW; - moved to XephyrContext */
/* RESTYPE XRT_PIXMAP; - moved to XephyrContext */
/* REMOVED: RESTYPE XRT_GC; - moved to XephyrContext */
/* REMOVED: RESTYPE XRT_COLORMAP; - moved to XephyrContext */

static Bool VisualsEqual(VisualPtr, ScreenPtr, VisualPtr);
XineramaVisualsEqualProcPtr XineramaVisualsEqualPtr = &VisualsEqual;

/*
 *	Function prototypes
 */

static int panoramiXGeneration;
static int ProcPanoramiXDispatch(ClientPtr client);

static void PanoramiXResetProc(ExtensionEntry *);

/*
 *	External references for functions and data variables
 */

#include "panoramiXh.h"

/* int (*SavedProcVector[256]) (ClientPtr client) = {
NULL,}; */

// static DevPrivateKeyRec PanoramiXGCKeyRec;

#define PanoramiXGCKey(pGC) (&(pGC)->pScreen->context->PanoramiXGCKeyRec)
// static DevPrivateKeyRec PanoramiXScreenKeyRec;

#define PanoramiXScreenKey(pScreen) (&(pScreen)->context->PanoramiXScreenKeyRec)

typedef struct {
    DDXPointRec clipOrg;
    DDXPointRec patOrg;
    const GCFuncs *wrapFuncs;
} PanoramiXGCRec, *PanoramiXGCPtr;

typedef struct {
    CreateGCProcPtr CreateGC;
    CloseScreenProcPtr CloseScreen;
} PanoramiXScreenRec, *PanoramiXScreenPtr;

static void XineramaValidateGC(GCPtr, unsigned long, DrawablePtr);
static void XineramaChangeGC(GCPtr, unsigned long);
static void XineramaCopyGC(GCPtr, unsigned long, GCPtr);
static void XineramaDestroyGC(GCPtr);
static void XineramaChangeClip(GCPtr, int, void *, int);
static void XineramaDestroyClip(GCPtr);
static void XineramaCopyClip(GCPtr, GCPtr);

static const GCFuncs XineramaGCFuncs = {
    XineramaValidateGC, XineramaChangeGC, XineramaCopyGC, XineramaDestroyGC,
    XineramaChangeClip, XineramaDestroyClip, XineramaCopyClip
};

#define Xinerama_GC_FUNC_PROLOGUE(pGC)\
    PanoramiXGCPtr  pGCPriv = (PanoramiXGCPtr) \
	dixLookupPrivate(&(pGC)->devPrivates, PanoramiXGCKey(pGC)); \
    (pGC)->funcs = pGCPriv->wrapFuncs;

#define Xinerama_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &XineramaGCFuncs;

static Bool
XineramaCloseScreen(ScreenPtr pScreen)
{
    PanoramiXScreenPtr pScreenPriv = (PanoramiXScreenPtr)
        dixLookupPrivate(&pScreen->devPrivates, PanoramiXScreenKey(pScreen));

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->CreateGC = pScreenPriv->CreateGC;

    if (pScreen->myNum == 0)
        RegionUninit(&pScreen->context->PanoramiXScreenRegion);

    free(pScreenPriv);

    return (*pScreen->CloseScreen) (pScreen);
}

static Bool
XineramaCreateGC(GCPtr pGC)
{
    ScreenPtr pScreen = pGC->pScreen;
    PanoramiXScreenPtr pScreenPriv = (PanoramiXScreenPtr)
        dixLookupPrivate(&pScreen->devPrivates, PanoramiXScreenKey(pScreen));
    Bool ret;

    pScreen->CreateGC = pScreenPriv->CreateGC;
    if ((ret = (*pScreen->CreateGC) (pGC))) {
        PanoramiXGCPtr pGCPriv = (PanoramiXGCPtr)
            dixLookupPrivate(&pGC->devPrivates, PanoramiXGCKey(pGC));

        pGCPriv->wrapFuncs = pGC->funcs;
        pGC->funcs = &XineramaGCFuncs;

        pGCPriv->clipOrg.x = pGC->clipOrg.x;
        pGCPriv->clipOrg.y = pGC->clipOrg.y;
        pGCPriv->patOrg.x = pGC->patOrg.x;
        pGCPriv->patOrg.y = pGC->patOrg.y;
    }
    pScreen->CreateGC = XineramaCreateGC;

    return ret;
}

static void
XineramaValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDraw)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);

    if ((pDraw->type == DRAWABLE_WINDOW) && !(((WindowPtr) pDraw)->parent)) {
        /* the root window */
        int x_off = pGC->pScreen->x;
        int y_off = pGC->pScreen->y;
        int new_val;

        new_val = pGCPriv->clipOrg.x - x_off;
        if (pGC->clipOrg.x != new_val) {
            pGC->clipOrg.x = new_val;
            changes |= GCClipXOrigin;
        }
        new_val = pGCPriv->clipOrg.y - y_off;
        if (pGC->clipOrg.y != new_val) {
            pGC->clipOrg.y = new_val;
            changes |= GCClipYOrigin;
        }
        new_val = pGCPriv->patOrg.x - x_off;
        if (pGC->patOrg.x != new_val) {
            pGC->patOrg.x = new_val;
            changes |= GCTileStipXOrigin;
        }
        new_val = pGCPriv->patOrg.y - y_off;
        if (pGC->patOrg.y != new_val) {
            pGC->patOrg.y = new_val;
            changes |= GCTileStipYOrigin;
        }
    }
    else {
        if (pGC->clipOrg.x != pGCPriv->clipOrg.x) {
            pGC->clipOrg.x = pGCPriv->clipOrg.x;
            changes |= GCClipXOrigin;
        }
        if (pGC->clipOrg.y != pGCPriv->clipOrg.y) {
            pGC->clipOrg.y = pGCPriv->clipOrg.y;
            changes |= GCClipYOrigin;
        }
        if (pGC->patOrg.x != pGCPriv->patOrg.x) {
            pGC->patOrg.x = pGCPriv->patOrg.x;
            changes |= GCTileStipXOrigin;
        }
        if (pGC->patOrg.y != pGCPriv->patOrg.y) {
            pGC->patOrg.y = pGCPriv->patOrg.y;
            changes |= GCTileStipYOrigin;
        }
    }

    (*pGC->funcs->ValidateGC) (pGC, changes, pDraw);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaDestroyGC(GCPtr pGC)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->DestroyGC) (pGC);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaChangeGC(GCPtr pGC, unsigned long mask)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);

    if (mask & GCTileStipXOrigin)
        pGCPriv->patOrg.x = pGC->patOrg.x;
    if (mask & GCTileStipYOrigin)
        pGCPriv->patOrg.y = pGC->patOrg.y;
    if (mask & GCClipXOrigin)
        pGCPriv->clipOrg.x = pGC->clipOrg.x;
    if (mask & GCClipYOrigin)
        pGCPriv->clipOrg.y = pGC->clipOrg.y;

    (*pGC->funcs->ChangeGC) (pGC, mask);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    PanoramiXGCPtr pSrcPriv = (PanoramiXGCPtr)
        dixLookupPrivate(&pGCSrc->devPrivates, PanoramiXGCKey(pGCSrc));

    Xinerama_GC_FUNC_PROLOGUE(pGCDst);

    if (mask & GCTileStipXOrigin)
        pGCPriv->patOrg.x = pSrcPriv->patOrg.x;
    if (mask & GCTileStipYOrigin)
        pGCPriv->patOrg.y = pSrcPriv->patOrg.y;
    if (mask & GCClipXOrigin)
        pGCPriv->clipOrg.x = pSrcPriv->clipOrg.x;
    if (mask & GCClipYOrigin)
        pGCPriv->clipOrg.y = pSrcPriv->clipOrg.y;

    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    Xinerama_GC_FUNC_EPILOGUE(pGCDst);
}

static void
XineramaChangeClip(GCPtr pGC, int type, void *pvalue, int nrects)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

static void
XineramaCopyClip(GCPtr pgcDst, GCPtr pgcSrc)
{
    Xinerama_GC_FUNC_PROLOGUE(pgcDst);
    (*pgcDst->funcs->CopyClip) (pgcDst, pgcSrc);
    Xinerama_GC_FUNC_EPILOGUE(pgcDst);
}

static void
XineramaDestroyClip(GCPtr pGC)
{
    Xinerama_GC_FUNC_PROLOGUE(pGC);
    (*pGC->funcs->DestroyClip) (pGC);
    Xinerama_GC_FUNC_EPILOGUE(pGC);
}

int
XineramaDeleteResource(void *data, XID id, XephyrContext* context)
{
    free(data);
    return 1;
}

typedef struct {
    int screen;
    int id;
} PanoramiXSearchData;

static Bool
XineramaFindIDByScrnum(void *resource, XID id, void *privdata)
{
    PanoramiXRes *res = (PanoramiXRes *) resource;
    PanoramiXSearchData *data = (PanoramiXSearchData *) privdata;

    return res->info[data->screen].id == data->id;
}

PanoramiXRes *
PanoramiXFindIDByScrnum(RESTYPE type, XID id, int screen, XephyrContext* context)
{
    PanoramiXSearchData data;
    void *val;

    if (!screen) {
        dixLookupResourceByType(&val, id, type, context->serverClient, DixReadAccess, context);
        return val;
    }

    data.screen = screen;
    data.id = id;

    return LookupClientResourceComplex(context->clients[CLIENT_ID(id)], type,
                                       XineramaFindIDByScrnum, &data, context);
}

typedef struct _connect_callback_list {
    void (*func) (XephyrContext*);
    struct _connect_callback_list *next;
} XineramaConnectionCallbackList;

// REMOVED: static XineramaConnectionCallbackList *ConnectionCallbackList = NULL; - moved to XephyrContext

Bool
XineramaRegisterConnectionBlockCallback(void (*func) (XephyrContext*), XephyrContext* context)
{
    XineramaConnectionCallbackList *newlist;

    if (!(newlist = malloc(sizeof(XineramaConnectionCallbackList))))
        return FALSE;

    newlist->next = context->ConnectionCallbackList;
    newlist->func = func;
    context->ConnectionCallbackList = newlist;

    return TRUE;
}

static void
XineramaInitData(XephyrContext* context)
{
    int i, w, h;

    RegionNull(&context->PanoramiXScreenRegion, context);
    FOR_NSCREENS(i) {
        BoxRec TheBox;
        RegionRec ScreenRegion;

        ScreenPtr pScreen = context->screenInfo.screens[i];

        TheBox.x1 = pScreen->x;
        TheBox.x2 = TheBox.x1 + pScreen->width;
        TheBox.y1 = pScreen->y;
        TheBox.y2 = TheBox.y1 + pScreen->height;

        RegionInit(&ScreenRegion, &TheBox, 1, context);
        RegionUnion(&context->PanoramiXScreenRegion, &context->PanoramiXScreenRegion,
                    &ScreenRegion);
        RegionUninit(&ScreenRegion);
    }

    context->PanoramiXPixWidth = context->screenInfo.screens[0]->x + context->screenInfo.screens[0]->width;
    context->PanoramiXPixHeight =
        context->screenInfo.screens[0]->y + context->screenInfo.screens[0]->height;

    FOR_NSCREENS_FORWARD_SKIP(i) {
        ScreenPtr pScreen = context->screenInfo.screens[i];

        w = pScreen->x + pScreen->width;
        h = pScreen->y + pScreen->height;

        if (context->PanoramiXPixWidth < w)
            context->PanoramiXPixWidth = w;
        if (context->PanoramiXPixHeight < h)
            context->PanoramiXPixHeight = h;
    }
}

void
XineramaReinitData(XephyrContext* context)
{
    RegionUninit(&context->PanoramiXScreenRegion);
    XineramaInitData(context);
}

/*
 *	PanoramiXExtensionInit():
 *		Called from InitExtensions in main().
 *		Register PanoramiXeen Extension
 *		Initialize global variables.
 */

void
PanoramiXExtensionInit(XephyrContext* context)
{
    int i;
    Bool success = FALSE;
    ExtensionEntry *extEntry;
    ScreenPtr pScreen = context->screenInfo.screens[0];
    PanoramiXScreenPtr pScreenPriv;


    if (noPanoramiXExtension)
        return;

    if (!dixRegisterPrivateKey(&context->PanoramiXScreenKeyRec, PRIVATE_SCREEN, 0, context)) {
        noPanoramiXExtension = TRUE;
        return;
    }

    if (!dixRegisterPrivateKey
        (&context->PanoramiXGCKeyRec, PRIVATE_GC, sizeof(PanoramiXGCRec), context)) {
        noPanoramiXExtension = TRUE;
        return;
    }

    context->PanoramiXNumScreens = context->screenInfo.numScreens;
    if (context->PanoramiXNumScreens == 1) {     /* Only 1 screen        */
        noPanoramiXExtension = TRUE;
        return;
    }

    while (panoramiXGeneration != context->serverGeneration) {
        extEntry = AddExtension(PANORAMIX_PROTOCOL_NAME, 0, 0,
                                ProcPanoramiXDispatch,
                                SProcPanoramiXDispatch, PanoramiXResetProc,
                                StandardMinorOpcode, context);
        if (!extEntry)
            break;

        /*
         *      First make sure all the basic allocations succeed.  If not,
         *      run in non-PanoramiXeen mode.
         */

        FOR_NSCREENS(i) {
            pScreen = context->screenInfo.screens[i];
            pScreenPriv = malloc(sizeof(PanoramiXScreenRec));
            dixSetPrivate(&pScreen->devPrivates, PanoramiXScreenKey(pScreen),
                          pScreenPriv);
            if (!pScreenPriv) {
                noPanoramiXExtension = TRUE;
                return;
            }

            pScreenPriv->CreateGC = pScreen->CreateGC;
            pScreenPriv->CloseScreen = pScreen->CloseScreen;

            pScreen->CreateGC = XineramaCreateGC;
            pScreen->CloseScreen = XineramaCloseScreen;
        }

        context->XRC_DRAWABLE = CreateNewResourceClass(context);
        context->XRT_WINDOW = CreateNewResourceType(XineramaDeleteResource,
                                           "XineramaWindow", context);
        if (context->XRT_WINDOW)
            context->XRT_WINDOW |= context->XRC_DRAWABLE;
        context->XRT_PIXMAP = CreateNewResourceType(XineramaDeleteResource,
                                           "XineramaPixmap", context);
        if (context->XRT_PIXMAP)
            context->XRT_PIXMAP |= context->XRC_DRAWABLE;
        context->XRT_GC = CreateNewResourceType(XineramaDeleteResource, "XineramaGC", context);
        context->XRT_COLORMAP = CreateNewResourceType(XineramaDeleteResource,
                                             "XineramaColormap", context);

        if (context->XRT_WINDOW && context->XRT_PIXMAP && context->XRT_GC && context->XRT_COLORMAP) {
            panoramiXGeneration = context->serverGeneration;
            success = TRUE;
        }
        SetResourceTypeErrorValue(context->XRT_WINDOW, BadWindow, context);
        SetResourceTypeErrorValue(context->XRT_PIXMAP, BadPixmap, context);
        SetResourceTypeErrorValue(context->XRT_GC, BadGC, context);
        SetResourceTypeErrorValue(context->XRT_COLORMAP, BadColor, context);
    }

    if (!success) {
        noPanoramiXExtension = TRUE;
        ErrorF(PANORAMIX_PROTOCOL_NAME " extension failed to initialize\n", context);
        return;
    }

    XineramaInitData(context);

    /*
     *  Put our processes into the ProcVector
     */

    for (i = 256; i--;)
        context->SavedProcVector[i] = ProcVector[i];

    ProcVector[X_CreateWindow] = PanoramiXCreateWindow;
    ProcVector[X_ChangeWindowAttributes] = PanoramiXChangeWindowAttributes;
    ProcVector[X_DestroyWindow] = PanoramiXDestroyWindow;
    ProcVector[X_DestroySubwindows] = PanoramiXDestroySubwindows;
    ProcVector[X_ChangeSaveSet] = PanoramiXChangeSaveSet;
    ProcVector[X_ReparentWindow] = PanoramiXReparentWindow;
    ProcVector[X_MapWindow] = PanoramiXMapWindow;
    ProcVector[X_MapSubwindows] = PanoramiXMapSubwindows;
    ProcVector[X_UnmapWindow] = PanoramiXUnmapWindow;
    ProcVector[X_UnmapSubwindows] = PanoramiXUnmapSubwindows;
    ProcVector[X_ConfigureWindow] = PanoramiXConfigureWindow;
    ProcVector[X_CirculateWindow] = PanoramiXCirculateWindow;
    ProcVector[X_GetGeometry] = PanoramiXGetGeometry;
    ProcVector[X_TranslateCoords] = PanoramiXTranslateCoords;
    ProcVector[X_CreatePixmap] = PanoramiXCreatePixmap;
    ProcVector[X_FreePixmap] = PanoramiXFreePixmap;
    ProcVector[X_CreateGC] = PanoramiXCreateGC;
    ProcVector[X_ChangeGC] = PanoramiXChangeGC;
    ProcVector[X_CopyGC] = PanoramiXCopyGC;
    ProcVector[X_SetDashes] = PanoramiXSetDashes;
    ProcVector[X_SetClipRectangles] = PanoramiXSetClipRectangles;
    ProcVector[X_FreeGC] = PanoramiXFreeGC;
    ProcVector[X_ClearArea] = PanoramiXClearToBackground;
    ProcVector[X_CopyArea] = PanoramiXCopyArea;
    ProcVector[X_CopyPlane] = PanoramiXCopyPlane;
    ProcVector[X_PolyPoint] = PanoramiXPolyPoint;
    ProcVector[X_PolyLine] = PanoramiXPolyLine;
    ProcVector[X_PolySegment] = PanoramiXPolySegment;
    ProcVector[X_PolyRectangle] = PanoramiXPolyRectangle;
    ProcVector[X_PolyArc] = PanoramiXPolyArc;
    ProcVector[X_FillPoly] = PanoramiXFillPoly;
    ProcVector[X_PolyFillRectangle] = PanoramiXPolyFillRectangle;
    ProcVector[X_PolyFillArc] = PanoramiXPolyFillArc;
    ProcVector[X_PutImage] = PanoramiXPutImage;
    ProcVector[X_GetImage] = PanoramiXGetImage;
    ProcVector[X_PolyText8] = PanoramiXPolyText8;
    ProcVector[X_PolyText16] = PanoramiXPolyText16;
    ProcVector[X_ImageText8] = PanoramiXImageText8;
    ProcVector[X_ImageText16] = PanoramiXImageText16;
    ProcVector[X_CreateColormap] = PanoramiXCreateColormap;
    ProcVector[X_FreeColormap] = PanoramiXFreeColormap;
    ProcVector[X_CopyColormapAndFree] = PanoramiXCopyColormapAndFree;
    ProcVector[X_InstallColormap] = PanoramiXInstallColormap;
    ProcVector[X_UninstallColormap] = PanoramiXUninstallColormap;
    ProcVector[X_AllocColor] = PanoramiXAllocColor;
    ProcVector[X_AllocNamedColor] = PanoramiXAllocNamedColor;
    ProcVector[X_AllocColorCells] = PanoramiXAllocColorCells;
    ProcVector[X_AllocColorPlanes] = PanoramiXAllocColorPlanes;
    ProcVector[X_FreeColors] = PanoramiXFreeColors;
    ProcVector[X_StoreColors] = PanoramiXStoreColors;
    ProcVector[X_StoreNamedColor] = PanoramiXStoreNamedColor;

    PanoramiXRenderInit(context);
    PanoramiXFixesInit(context);
    PanoramiXDamageInit(context);
#ifdef COMPOSITE
    PanoramiXCompositeInit(context);
#endif

}

Bool
PanoramiXCreateConnectionBlock(XephyrContext* context)
{
    int i, j, length;
    Bool disable_backing_store = FALSE;
    int old_width, old_height;
    float width_mult, height_mult;
    xWindowRoot *root;
    xVisualType *visual;
    xDepth *depth;
    VisualPtr pVisual;
    ScreenPtr pScreen;

    /*
     *  Do normal CreateConnectionBlock but faking it for only one screen
     */

    if (!PanoramiXNumDepths) {
        ErrorF("Xinerama error: No common visuals\n", context);
        return FALSE;
    }

    for (i = 1; i < context->screenInfo.numScreens; i++) {
        pScreen = context->screenInfo.screens[i];
        if (pScreen->rootDepth != context->screenInfo.screens[0]->rootDepth) {
            ErrorF("Xinerama error: Root window depths differ\n", context);
            return FALSE;
        }
        if (pScreen->backingStoreSupport !=
            context->screenInfo.screens[0]->backingStoreSupport)
            disable_backing_store = TRUE;
    }

    if (disable_backing_store) {
        for (i = 0; i < context->screenInfo.numScreens; i++) {
            pScreen = context->screenInfo.screens[i];
            pScreen->backingStoreSupport = NotUseful;
        }
    }

    i = context->screenInfo.numScreens;
    context->screenInfo.numScreens = 1;
    if (!CreateConnectionBlock(context)) {
        context->screenInfo.numScreens = i;
        return FALSE;
    }

    context->screenInfo.numScreens = i;

    root = (xWindowRoot *) (context->ConnectionInfo + context->connBlockScreenStart);
    length = context->connBlockScreenStart + sizeof(xWindowRoot);

    /* overwrite the connection block */
    root->nDepths = PanoramiXNumDepths;

    for (i = 0; i < PanoramiXNumDepths; i++) {
        depth = (xDepth *) (context->ConnectionInfo + length);
        depth->depth = PanoramiXDepths[i].depth;
        depth->nVisuals = PanoramiXDepths[i].numVids;
        length += sizeof(xDepth);
        visual = (xVisualType *) (context->ConnectionInfo + length);

        for (j = 0; j < depth->nVisuals; j++, visual++) {
            visual->visualID = PanoramiXDepths[i].vids[j];

            for (pVisual = context->PanoramiXVisuals;
                 pVisual->vid != visual->visualID; pVisual++);

            visual->class = pVisual->class;
            visual->bitsPerRGB = pVisual->bitsPerRGBValue;
            visual->colormapEntries = pVisual->ColormapEntries;
            visual->redMask = pVisual->redMask;
            visual->greenMask = pVisual->greenMask;
            visual->blueMask = pVisual->blueMask;
        }

        length += (depth->nVisuals * sizeof(xVisualType));
    }

    context->connSetupPrefix.length = bytes_to_int32(length);

    for (i = 0; i < PanoramiXNumDepths; i++)
        free(PanoramiXDepths[i].vids);
    free(PanoramiXDepths);
    PanoramiXDepths = NULL;

    /*
     *  OK, change some dimensions so it looks as if it were one big screen
     */

    old_width = root->pixWidth;
    old_height = root->pixHeight;

    root->pixWidth = context->PanoramiXPixWidth;
    root->pixHeight = context->PanoramiXPixHeight;
    width_mult = (1.0 * root->pixWidth) / old_width;
    height_mult = (1.0 * root->pixHeight) / old_height;
    root->mmWidth *= width_mult;
    root->mmHeight *= height_mult;

    while (context->ConnectionCallbackList) {
        void *tmp;

        tmp = (void *) context->ConnectionCallbackList;
        (*((XineramaConnectionCallbackList*)context->ConnectionCallbackList)->func) (context);
        context->ConnectionCallbackList = ((XineramaConnectionCallbackList*)context->ConnectionCallbackList)->next;
        free(tmp);
    }

    return TRUE;
}

/*
 * This isn't just memcmp(), bitsPerRGBValue is skipped.  markv made that
 * change way back before xf86 4.0, but the comment for _why_ is a bit
 * opaque, so I'm not going to question it for now.
 *
 * This is probably better done as a screen hook so DBE/EVI/GLX can add
 * their own tests, and adding privates to VisualRec so they don't have to
 * do their own back-mapping.
 */
static Bool
VisualsEqual(VisualPtr a, ScreenPtr pScreenB, VisualPtr b)
{
    return ((a->class == b->class) &&
            (a->ColormapEntries == b->ColormapEntries) &&
            (a->nplanes == b->nplanes) &&
            (a->redMask == b->redMask) &&
            (a->greenMask == b->greenMask) &&
            (a->blueMask == b->blueMask) &&
            (a->offsetRed == b->offsetRed) &&
            (a->offsetGreen == b->offsetGreen) &&
            (a->offsetBlue == b->offsetBlue));
}

static void
PanoramiXMaybeAddDepth(DepthPtr pDepth, XephyrContext* context)
{
    ScreenPtr pScreen;
    int j, k;
    Bool found = FALSE;

    FOR_NSCREENS_FORWARD_SKIP(j) {
        pScreen = context->screenInfo.screens[j];
        for (k = 0; k < pScreen->numDepths; k++) {
            if (pScreen->allowedDepths[k].depth == pDepth->depth) {
                found = TRUE;
                break;
            }
        }
    }

    if (!found)
        return;

    j = PanoramiXNumDepths;
    PanoramiXNumDepths++;
    PanoramiXDepths = reallocarray(PanoramiXDepths,
                                   PanoramiXNumDepths, sizeof(DepthRec));
    PanoramiXDepths[j].depth = pDepth->depth;
    PanoramiXDepths[j].numVids = 0;
    PanoramiXDepths[j].vids = NULL;
}

static void
PanoramiXMaybeAddVisual(VisualPtr pVisual, XephyrContext* context)
{
    ScreenPtr pScreen;
    int j, k;
    Bool found = FALSE;

    FOR_NSCREENS_FORWARD_SKIP(j) {
        pScreen = context->screenInfo.screens[j];
        found = FALSE;

        for (k = 0; k < pScreen->numVisuals; k++) {
            VisualPtr candidate = &pScreen->visuals[k];

            if ((*XineramaVisualsEqualPtr) (pVisual, pScreen, candidate)
#ifdef GLXPROXY
                && glxMatchVisual(context->screenInfo.screens[0], pVisual, pScreen)
#endif
                ) {
                found = TRUE;
                break;
            }
        }

        if (!found)
            return;
    }

    /* found a matching visual on all screens, add it to the subset list */
    j = context->PanoramiXNumVisuals;
    context->PanoramiXNumVisuals++;
    context->PanoramiXVisuals = reallocarray(context->PanoramiXVisuals,
                                    context->PanoramiXNumVisuals, sizeof(VisualRec));

    memcpy(&context->PanoramiXVisuals[j], pVisual, sizeof(VisualRec));

    for (k = 0; k < PanoramiXNumDepths; k++) {
        if (PanoramiXDepths[k].depth == pVisual->nplanes) {
            PanoramiXDepths[k].vids = reallocarray(PanoramiXDepths[k].vids,
                                                   PanoramiXDepths[k].numVids + 1,
                                                   sizeof(VisualID));
            PanoramiXDepths[k].vids[PanoramiXDepths[k].numVids] = pVisual->vid;
            PanoramiXDepths[k].numVids++;
            break;
        }
    }
}

extern void
PanoramiXConsolidate(XephyrContext* context)
{
    int i;
    PanoramiXRes *root, *defmap, *saver;
    ScreenPtr pScreen = context->screenInfo.screens[0];
    DepthPtr pDepth = pScreen->allowedDepths;
    VisualPtr pVisual = pScreen->visuals;

    PanoramiXNumDepths = 0;
    context->PanoramiXNumVisuals = 0;

    for (i = 0; i < pScreen->numDepths; i++)
        PanoramiXMaybeAddDepth(pDepth++, context);

    for (i = 0; i < pScreen->numVisuals; i++)
        PanoramiXMaybeAddVisual(pVisual++, context);

    root = malloc(sizeof(PanoramiXRes));
    root->type = context->XRT_WINDOW;
    defmap = malloc(sizeof(PanoramiXRes));
    defmap->type = context->XRT_COLORMAP;
    saver = malloc(sizeof(PanoramiXRes));
    saver->type = context->XRT_WINDOW;

    FOR_NSCREENS(i) {
        ScreenPtr scr = context->screenInfo.screens[i];

        root->info[i].id = scr->root->drawable.id;
        root->u.win.class = InputOutput;
        root->u.win.root = TRUE;
        saver->info[i].id = scr->screensaver.wid;
        saver->u.win.class = InputOutput;
        saver->u.win.root = TRUE;
        defmap->info[i].id = scr->defColormap;
    }

    AddResource(root->info[0].id, context->XRT_WINDOW, root, context);
    AddResource(saver->info[0].id, context->XRT_WINDOW, saver, context);
    AddResource(defmap->info[0].id, context->XRT_COLORMAP, defmap, context);
}

VisualID
PanoramiXTranslateVisualID(int screen, VisualID orig, XephyrContext* context)
{
    ScreenPtr pOtherScreen = context->screenInfo.screens[screen];
    VisualPtr pVisual = NULL;
    int i;

    for (i = 0; i < context->PanoramiXNumVisuals; i++) {
        if (orig == context->PanoramiXVisuals[i].vid) {
            pVisual = &context->PanoramiXVisuals[i];
            break;
        }
    }

    if (!pVisual)
        return 0;

    /* if screen is 0, orig is already the correct visual ID */
    if (screen == 0)
        return orig;

    /* found the original, now translate it relative to the backend screen */
    for (i = 0; i < pOtherScreen->numVisuals; i++) {
        VisualPtr pOtherVisual = &pOtherScreen->visuals[i];

        if ((*XineramaVisualsEqualPtr) (pVisual, pOtherScreen, pOtherVisual))
            return pOtherVisual->vid;
    }

    return 0;
}

/*
 *	PanoramiXResetProc()
 *		Exit, deallocating as needed.
 */

static void
PanoramiXResetProc(ExtensionEntry * extEntry)
{
    int i;
    XephyrContext* context = (XephyrContext*)extEntry->extPrivate;

    PanoramiXRenderReset(context);
    PanoramiXFixesReset(context);
    PanoramiXDamageReset(context);
#ifdef COMPOSITE
    PanoramiXCompositeReset(context);
#endif
    context->screenInfo.numScreens = context->PanoramiXNumScreens;
    for (i = 256; i--;)
        ProcVector[i] = context->SavedProcVector[i];
}

int
ProcPanoramiXQueryVersion(ClientPtr client)
{
    /* REQUEST(xPanoramiXQueryVersionReq); */
    xPanoramiXQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_PANORAMIX_MAJOR_VERSION,
        .minorVersion = SERVER_PANORAMIX_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xPanoramiXQueryVersionReq);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xPanoramiXQueryVersionReply), &rep);
    return Success;
}

int
ProcPanoramiXGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);
    WindowPtr pWin;
    xPanoramiXGetStateReply rep;
    int rc;

    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep = (xPanoramiXGetStateReply) {
        .type = X_Reply,
        .state = !noPanoramiXExtension,
        .sequenceNumber = client->sequence,
        .length = 0,
        .window = stuff->window
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.window);
    }
    WriteToClient(client, sizeof(xPanoramiXGetStateReply), &rep);
    return Success;

}

int
ProcPanoramiXGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);
    WindowPtr pWin;
    xPanoramiXGetScreenCountReply rep;
    int rc;
    XephyrContext *context = client->context;

    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep = (xPanoramiXGetScreenCountReply) {
        .type = X_Reply,
        .ScreenCount = context->PanoramiXNumScreens,
        .sequenceNumber = client->sequence,
        .length = 0,
        .window = stuff->window
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.window);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenCountReply), &rep);
    return Success;
}

int
ProcPanoramiXGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);
    WindowPtr pWin;
    xPanoramiXGetScreenSizeReply rep;
    int rc;
    XephyrContext *context = client->context;

    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);

    if (stuff->screen >= context->PanoramiXNumScreens)
        return BadMatch;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep = (xPanoramiXGetScreenSizeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
    /* screen dimensions */
        .width = client->context->screenInfo.screens[stuff->screen]->width,
        .height = client->context->screenInfo.screens[stuff->screen]->height,
        .window = stuff->window,
        .screen = stuff->screen
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.width);
        swapl(&rep.height);
        swapl(&rep.window);
        swapl(&rep.screen);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenSizeReply), &rep);
    return Success;
}

int
ProcXineramaIsActive(ClientPtr client)
{
    /* REQUEST(xXineramaIsActiveReq); */
    xXineramaIsActiveReply rep;
    XephyrContext *context = client->context;

    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);

    rep = (xXineramaIsActiveReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
#if 1
        /* The following hack fools context->clients into thinking that Xinerama
         * is disabled even though it is not. */
        .state = !noPanoramiXExtension && !context->PanoramiXExtensionDisabledHack
#else
        .state = !noPanoramiXExtension;
#endif
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.state);
    }
    WriteToClient(client, sizeof(xXineramaIsActiveReply), &rep);
    return Success;
}

int
ProcXineramaQueryScreens(ClientPtr client)
{
    /* REQUEST(xXineramaQueryScreensReq); */
    XephyrContext *context = client->context;
    CARD32 number = (noPanoramiXExtension) ? 0 : context->PanoramiXNumScreens;
    xXineramaQueryScreensReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(number * sz_XineramaScreenInfo),
        .number = number
    };

    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.number);
    }
    WriteToClient(client, sizeof(xXineramaQueryScreensReply), &rep);

    if (!noPanoramiXExtension) {
        xXineramaScreenInfo scratch;
        int i;

        FOR_NSCREENS(i) {
            scratch.x_org = client->context->screenInfo.screens[i]->x;
            scratch.y_org = client->context->screenInfo.screens[i]->y;
            scratch.width = client->context->screenInfo.screens[i]->width;
            scratch.height = client->context->screenInfo.screens[i]->height;

            if (client->swapped) {
                swaps(&scratch.x_org);
                swaps(&scratch.y_org);
                swaps(&scratch.width);
                swaps(&scratch.height);
            }
            WriteToClient(client, sz_XineramaScreenInfo, &scratch);
        }
    }

    return Success;
}

static int
ProcPanoramiXDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return ProcPanoramiXQueryVersion(client);
    case X_PanoramiXGetState:
        return ProcPanoramiXGetState(client);
    case X_PanoramiXGetScreenCount:
        return ProcPanoramiXGetScreenCount(client);
    case X_PanoramiXGetScreenSize:
        return ProcPanoramiXGetScreenSize(client);
    case X_XineramaIsActive:
        return ProcXineramaIsActive(client);
    case X_XineramaQueryScreens:
        return ProcXineramaQueryScreens(client);
    }
    return BadRequest;
}

#if X_BYTE_ORDER == X_LITTLE_ENDIAN
#define SHIFT_L(v,s) (v) << (s)
#define SHIFT_R(v,s) (v) >> (s)
#else
#define SHIFT_L(v,s) (v) >> (s)
#define SHIFT_R(v,s) (v) << (s)
#endif

static void
CopyBits(char *dst, int shiftL, char *src, int bytes)
{
    /* Just get it to work.  Worry about speed later */
    int shiftR = 8 - shiftL;

    while (bytes--) {
        *dst |= SHIFT_L(*src, shiftL);
        *(dst + 1) |= SHIFT_R(*src, shiftR);
        dst++;
        src++;
    }
}

/* Caution.  This doesn't support 2 and 4 bpp formats.  We expect
   1 bpp and planar data to be already cleared when presented
   to this function */

void
XineramaGetImageData(DrawablePtr *pDrawables,
                     int left,
                     int top,
                     int width,
                     int height,
                     unsigned int format,
                     unsigned long planemask,
                     char *data, int pitch, Bool isRoot,
                     XephyrContext* context)
{
    RegionRec SrcRegion, ScreenRegion, GrabRegion;
    BoxRec SrcBox, *pbox;
    int x, y, w, h, i, j, nbox, size, sizeNeeded, ScratchPitch, inOut, depth;
    DrawablePtr pDraw = pDrawables[0];
    char *ScratchMem = NULL;

    size = 0;

    /* find box in logical screen space */
    SrcBox.x1 = left;
    SrcBox.y1 = top;
    if (!isRoot) {
        SrcBox.x1 += pDraw->x + context->screenInfo.screens[0]->x;
        SrcBox.y1 += pDraw->y + context->screenInfo.screens[0]->y;
    }
    SrcBox.x2 = SrcBox.x1 + width;
    SrcBox.y2 = SrcBox.y1 + height;

    RegionInit(&SrcRegion, &SrcBox, 1, pDraw->pScreen->context);
    RegionNull(&GrabRegion, pDraw->pScreen->context);

    depth = (format == XYPixmap) ? 1 : pDraw->depth;

    FOR_NSCREENS(i) {
        BoxRec TheBox;
        ScreenPtr pScreen;

        pDraw = pDrawables[i];
        pScreen = pDraw->pScreen;

        TheBox.x1 = pScreen->x;
        TheBox.x2 = TheBox.x1 + pScreen->width;
        TheBox.y1 = pScreen->y;
        TheBox.y2 = TheBox.y1 + pScreen->height;

        RegionInit(&ScreenRegion, &TheBox, 1, pScreen->context);
        inOut = RegionContainsRect(&ScreenRegion, &SrcBox);
        if (inOut == rgnPART)
            RegionIntersect(&GrabRegion, &SrcRegion, &ScreenRegion);
        RegionUninit(&ScreenRegion);

        if (inOut == rgnIN) {
            (*pScreen->GetImage) (pDraw,
                                  SrcBox.x1 - pDraw->x -
                                  context->screenInfo.screens[i]->x,
                                  SrcBox.y1 - pDraw->y -
                                  context->screenInfo.screens[i]->y, width, height,
                                  format, planemask, data);
            break;
        }
        else if (inOut == rgnOUT)
            continue;

        nbox = RegionNumRects(&GrabRegion);

        if (nbox) {
            pbox = RegionRects(&GrabRegion);

            while (nbox--) {
                w = pbox->x2 - pbox->x1;
                h = pbox->y2 - pbox->y1;
                ScratchPitch = PixmapBytePad(w, depth, context);
                sizeNeeded = ScratchPitch * h;

                if (sizeNeeded > size) {
                    char *tmpdata = ScratchMem;

                    ScratchMem = realloc(ScratchMem, sizeNeeded);
                    if (ScratchMem)
                        size = sizeNeeded;
                    else {
                        ScratchMem = tmpdata;
                        break;
                    }
                }

                x = pbox->x1 - pDraw->x - context->screenInfo.screens[i]->x;
                y = pbox->y1 - pDraw->y - context->screenInfo.screens[i]->y;

                (*pScreen->GetImage) (pDraw, x, y, w, h,
                                      format, planemask, ScratchMem);

                /* copy the memory over */

                if (depth == 1) {
                    int k, shift, leftover, index, index2;

                    x = pbox->x1 - SrcBox.x1;
                    y = pbox->y1 - SrcBox.y1;
                    shift = x & 7;
                    x >>= 3;
                    leftover = w & 7;
                    w >>= 3;

                    /* clean up the edge */
                    if (leftover) {
                        int mask = (1 << leftover) - 1;

                        for (j = h, k = w; j--; k += ScratchPitch)
                            ScratchMem[k] &= mask;
                    }

                    for (j = 0, index = (pitch * y) + x, index2 = 0; j < h;
                         j++, index += pitch, index2 += ScratchPitch) {
                        if (w) {
                            if (!shift)
                                memcpy(data + index, ScratchMem + index2, w);
                            else
                                CopyBits(data + index, shift,
                                         ScratchMem + index2, w);
                        }

                        if (leftover) {
                            data[index + w] |=
                                SHIFT_L(ScratchMem[index2 + w], shift);
                            if ((shift + leftover) > 8)
                                data[index + w + 1] |=
                                    SHIFT_R(ScratchMem[index2 + w],
                                            (8 - shift));
                        }
                    }
                }
                else {
                    j = BitsPerPixel(depth, context) >> 3;
                    x = (pbox->x1 - SrcBox.x1) * j;
                    y = pbox->y1 - SrcBox.y1;
                    w *= j;

                    for (j = 0; j < h; j++) {
                        memcpy(data + (pitch * (y + j)) + x,
                               ScratchMem + (ScratchPitch * j), w);
                    }
                }
                pbox++;
            }

            RegionSubtract(&SrcRegion, &SrcRegion, &GrabRegion);
            if (!RegionNotEmpty(&SrcRegion))
                break;
        }

    }

    free(ScratchMem);

    RegionUninit(&SrcRegion);
    RegionUninit(&GrabRegion);
}
