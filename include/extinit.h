/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/*
 * Copyright (C) 1994-2003 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
 * NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall not
 * be used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from the XFree86 Project.
 */

#ifndef EXTINIT_H
#define EXTINIT_H

#include "extnsionst.h"

#ifdef COMPOSITE
/* extern _X_EXPORT Bool noCompositeExtension; - moved to XephyrContext */
extern void CompositeExtensionInit(XephyrContext* context);
#endif

#ifdef DAMAGE
/* extern _X_EXPORT Bool noDamageExtension; - moved to XephyrContext */
extern void DamageExtensionInit(XephyrContext* context);
#endif

#if defined(DBE)
/* extern _X_EXPORT Bool noDbeExtension; - moved to XephyrContext */
extern void DbeExtensionInit(XephyrContext* context);
#endif

#if defined(DPMSExtension)
extern _X_EXPORT Bool noDPMSExtension;
extern void DPMSExtensionInit(XephyrContext* context);
#endif

/* extern Bool noGEExtension; - moved to XephyrContext */
extern void GEExtensionInit(XephyrContext* context);

#ifdef GLXEXT
/* extern _X_EXPORT Bool noGlxExtension; - moved to XephyrContext */
extern void GlxExtensionInit(XephyrContext* context);
#endif

#ifdef PANORAMIX
extern _X_EXPORT Bool noPanoramiXExtension;
extern void PanoramiXExtensionInit(XephyrContext* context);
#endif

#ifdef RANDR
/* extern _X_EXPORT Bool noRRExtension; - moved to XephyrContext */
extern void RRExtensionInit(XephyrContext* context);
#endif

#if defined(XRECORD)
extern void RecordExtensionInit(XephyrContext* context);
#endif

/* extern _X_EXPORT Bool noRenderExtension; - moved to XephyrContext */
extern void RenderExtensionInit(XephyrContext* context);

#if defined(RES)
/* extern _X_EXPORT Bool noResExtension; - moved to XephyrContext */
extern void ResExtensionInit(XephyrContext* context);
#endif

#if defined(SCREENSAVER)
/* extern _X_EXPORT Bool noScreenSaverExtension; - moved to XephyrContext */
extern void ScreenSaverExtensionInit(XephyrContext* context);
#endif

extern void ShapeExtensionInit(XephyrContext* context);

#ifdef MITSHM
/* extern _X_EXPORT Bool noMITShmExtension; - moved to XephyrContext */
extern void ShmExtensionInit(XephyrContext* context);
#endif

extern void SyncExtensionInit(XephyrContext* context);

extern void XCMiscExtensionInit(XephyrContext* context);

#ifdef XCSECURITY
extern _X_EXPORT Bool noSecurityExtension;
extern void SecurityExtensionInit(XephyrContext* context);
#endif

#ifdef XF86BIGFONT
extern _X_EXPORT Bool noXFree86BigfontExtension;
extern void XFree86BigfontExtensionInit(void);
#endif

extern void BigReqExtensionInit(XephyrContext* context);

/* extern _X_EXPORT Bool noXFixesExtension; - moved to XephyrContext */
extern void XFixesExtensionInit(XephyrContext* context);

extern void XInputExtensionInit(XephyrContext* context);

extern void XkbExtensionInit(XephyrContext* context);

#if defined(XSELINUX)
extern _X_EXPORT Bool noSELinuxExtension;
extern void SELinuxExtensionInit(XephyrContext* context);
#endif

#ifdef XTEST
extern void XTestExtensionInit(XephyrContext* context);
#endif

#if defined(XV)
/* extern _X_EXPORT Bool noXvExtension; - moved to XephyrContext */
extern void XvExtensionInit(XephyrContext* context);
extern void XvMCExtensionInit(XephyrContext* context);
#endif

#if defined(DRI3)
extern void dri3_extension_init(void);
#endif

#if defined(PRESENT)
#include "presentext.h"
#endif

#endif
