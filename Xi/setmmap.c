#include "dix/context.h"
/************************************************************

Copyright 1989, 1998  The Open Group

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

Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/********************************************************************
 *
 *  Set modifier mapping for an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#include "exevents.h"
#include "exglobals.h"

#include "setmmap.h"

/***********************************************************************
 *
 * This procedure sets the modifier mapping for an extension device,
 * for context->clients on machines with a different byte ordering than the server.
 *
 */

int _X_COLD
SProcXSetDeviceModifierMapping(ClientPtr client)
{
    REQUEST(xSetDeviceModifierMappingReq);
    swaps(&stuff->length);
    return (ProcXSetDeviceModifierMapping(client));
}

/***********************************************************************
 *
 * Set the device Modifier mapping.
 *
 */

int
ProcXSetDeviceModifierMapping(ClientPtr client)
{
    int ret;
    xSetDeviceModifierMappingReply rep;
    DeviceIntPtr dev;

    REQUEST(xSetDeviceModifierMappingReq);
    REQUEST_AT_LEAST_SIZE(xSetDeviceModifierMappingReq);

    if (stuff->length != bytes_to_int32(sizeof(xSetDeviceModifierMappingReq)) +
        (stuff->numKeyPerModifier << 1))
        return BadLength;

    rep = (xSetDeviceModifierMappingReply) {
        .repType = X_Reply,
        .RepType = X_SetDeviceModifierMapping,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixManageAccess);
    if (ret != Success)
        return ret;

    ret = change_modmap(client, dev, (KeyCode *) &stuff[1],
                        stuff->numKeyPerModifier);
    if (ret == Success)
        ret = MappingSuccess;

    if (ret == MappingSuccess || ret == MappingBusy || ret == MappingFailed) {
        rep.success = ret;
        WriteReplyToClient(client, sizeof(xSetDeviceModifierMappingReply),
                           &rep);
    }
    else if (ret == -1) {
        return BadValue;
    }
    else {
        return ret;
    }

    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XSetDeviceModifierMapping function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXSetDeviceModifierMapping(ClientPtr client, int size,
                              xSetDeviceModifierMappingReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
