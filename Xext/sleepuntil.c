#include "dix/context.h"
/*
 *
Copyright 1992, 1998  The Open Group

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
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/* dixsleep.c - implement millisecond timeouts for X context->clients */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "sleepuntil.h"
#include <X11/X.h>
#include <X11/Xmd.h>
#include "misc.h"
#include "windowstr.h"
#include "dixstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

typedef struct _Sertafied {
    struct _Sertafied *next;
    TimeStamp revive;
    ClientPtr pClient;
    XID id;
    void (*notifyFunc) (ClientPtr /* client */ ,
                        void *    /* closure */
        );

    void *closure;
} SertafiedRec, *SertafiedPtr;

static SertafiedPtr pPending;
static RESTYPE SertafiedResType;
// static Bool BlockHandlerRegistered; // Moved to XephyrContext
static int SertafiedGeneration;

static void ClientAwaken(ClientPtr /* client */ ,
                         void *    /* closure */
    );
static int SertafiedDelete(void *  /* value */ ,
                           XID     /* id */,
                           XephyrContext* /* context */
    );
static void SertafiedBlockHandler(void *data,
                                  void *timeout);

static void SertafiedWakeupHandler(void *data,
                                   int i);

int
ClientSleepUntil(ClientPtr client,
                 TimeStamp *revive,
                 void (*notifyFunc) (ClientPtr, void *), void *closure)
{
    SertafiedPtr pRequest, pReq, pPrev;

    if (SertafiedGeneration != client->context->serverGeneration) {
        SertafiedResType = CreateNewResourceType(SertafiedDelete,
                                                 "ClientSleep", client->context);
        if (!SertafiedResType)
            return FALSE;
        SertafiedGeneration = client->context->serverGeneration;
        client->context->BlockHandlerRegistered = FALSE;
    }
    pRequest = malloc(sizeof(SertafiedRec));
    if (!pRequest)
        return FALSE;
    pRequest->pClient = client;
    pRequest->revive = *revive;
    pRequest->id = FakeClientID(client->index, client->context);
    pRequest->closure = closure;
    if (!client->context->BlockHandlerRegistered) {
        if (!RegisterBlockAndWakeupHandlers(SertafiedBlockHandler,
                                            SertafiedWakeupHandler,
                                            client->context)) {
            free(pRequest);
            return FALSE;
        }
        client->context->BlockHandlerRegistered = TRUE;
    }
    pRequest->notifyFunc = 0;
    if (!AddResource(pRequest->id, SertafiedResType, (void *) pRequest, client->context))
        return FALSE;
    if (!notifyFunc)
        notifyFunc = ClientAwaken;
    pRequest->notifyFunc = notifyFunc;
    /* Insert into time-ordered queue, with earliest activation time coming first. */
    pPrev = 0;
    for (pReq = pPending; pReq; pReq = pReq->next) {
        if (CompareTimeStamps(pReq->revive, *revive) == LATER)
            break;
        pPrev = pReq;
    }
    if (pPrev)
        pPrev->next = pRequest;
    else
        pPending = pRequest;
    pRequest->next = pReq;
    IgnoreClient(client);
    return TRUE;
}

static void
ClientAwaken(ClientPtr client, void *closure)
{
    AttendClient(client);
}

static int
SertafiedDelete(void *value, XID id, XephyrContext* context)
{
    SertafiedPtr pRequest = (SertafiedPtr) value;
    SertafiedPtr pReq, pPrev;

    pPrev = 0;
    for (pReq = pPending; pReq; pPrev = pReq, pReq = pReq->next)
        if (pReq == pRequest) {
            if (pPrev)
                pPrev->next = pReq->next;
            else
                pPending = pReq->next;
            break;
        }
    if (pRequest->notifyFunc)
        (*pRequest->notifyFunc) (pRequest->pClient, pRequest->closure);
    free(pRequest);
    return TRUE;
}

static void
SertafiedBlockHandler(void *data, void *wt)
{
    SertafiedPtr pReq, pNext;
    unsigned long delay;
    TimeStamp now;

    if (!pPending)
        return;
    now.milliseconds = GetTimeInMillis();
    now.months = pPending->pClient->context->currentTime.months;
    if ((int) (now.milliseconds - pPending->pClient->context->currentTime.milliseconds) < 0)
        now.months++;
    for (pReq = pPending; pReq; pReq = pNext) {
        pNext = pReq->next;
        if (CompareTimeStamps(pReq->revive, now) == LATER)
            break;
        FreeResource(pReq->id, RT_NONE, pReq->pClient->context);

        /* AttendClient() may have been called via the resource delete
         * function so a client may have input to be processed and so
         *  set delay to 0 to prevent blocking in WaitForSomething().
         */
        AdjustWaitForDelay(wt, 0);
    }
    pReq = pPending;
    if (!pReq)
        return;
    delay = pReq->revive.milliseconds - now.milliseconds;
    AdjustWaitForDelay(wt, delay);
}

static void
SertafiedWakeupHandler(void *data, int i)
{
    SertafiedPtr pReq, pNext;
    TimeStamp now;

    now.milliseconds = GetTimeInMillis();
    now.months = pPending->pClient->context->currentTime.months;
    if ((int) (now.milliseconds - pPending->pClient->context->currentTime.milliseconds) < 0)
        now.months++;
    for (pReq = pPending; pReq; pReq = pNext) {
        pNext = pReq->next;
        if (CompareTimeStamps(pReq->revive, now) == LATER)
            break;
        FreeResource(pReq->id, RT_NONE, pReq->pClient->context);
    }
    if (!pPending) {
        XephyrContext *context = (XephyrContext *)data;
        RemoveBlockAndWakeupHandlers(SertafiedBlockHandler,
                                     SertafiedWakeupHandler, context);
        context->BlockHandlerRegistered = FALSE;
    }
}
