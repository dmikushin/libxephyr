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

#ifndef SELECTION_H
#define SELECTION_H 1

#include "dixstruct.h"
#include "privates.h"

/*
 *  Selection data structures
 */

typedef struct _Selection {
    Atom selection;
    TimeStamp lastTimeChanged;
    Window window;
    WindowPtr pWin;
    ClientPtr client;
    struct _Selection *next;
    PrivateRec *devPrivates;
} Selection;

/*
 *  Selection API
 */

extern _X_EXPORT int dixLookupSelection(Selection ** result, Atom name,
                                        ClientPtr client, Mask access_mode);

extern _X_EXPORT Selection *CurrentSelections;

/* extern _X_EXPORT CallbackListPtr context->SelectionCallback; */

typedef enum {
    SelectionSetOwner,
    SelectionWindowDestroy,
    SelectionClientClose
} SelectionCallbackKind;

typedef struct {
    struct _Selection *selection;
    ClientPtr client;
    SelectionCallbackKind kind;
} SelectionInfoRec;

/*
 *  Selection server internals
 */

extern _X_EXPORT void InitSelections(XephyrContext* context);

extern _X_EXPORT void DeleteWindowFromAnySelections(WindowPtr pWin);

extern _X_EXPORT void DeleteClientFromAnySelections(ClientPtr client);

#endif                          /* SELECTION_H */
