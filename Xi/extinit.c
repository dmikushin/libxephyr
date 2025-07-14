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
 *  Dispatch routines and initialization routines for the X input extension.
 *
 */
#define	 NUMTYPES 15

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"
#include "gcstruct.h"           /* pointer for extnsionst.h */
#include "extnsionst.h"         /* extension entry   */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/geproto.h>
#include "geext.h"              /* extension interfaces for ge */

#include "dixevents.h"
#include "exevents.h"
#include "extinit.h"
#include "exglobals.h"
#include "swaprep.h"
#include "privates.h"
#include "protocol-versions.h"

/* modules local to Xi */
#include "allowev.h"
#include "chgdctl.h"
#include "chgfctl.h"
#include "chgkbd.h"
#include "chgprop.h"
#include "chgptr.h"
#include "closedev.h"
#include "devbell.h"
#include "getbmap.h"
#include "getdctl.h"
#include "getfctl.h"
#include "getfocus.h"
#include "getkmap.h"
#include "getmmap.h"
#include "getprop.h"
#include "getselev.h"
#include "getvers.h"
#include "grabdev.h"
#include "grabdevb.h"
#include "grabdevk.h"
#include "gtmotion.h"
#include "listdev.h"
#include "opendev.h"
#include "queryst.h"
#include "selectev.h"
#include "sendexev.h"
#include "chgkmap.h"
#include "setbmap.h"
#include "setdval.h"
#include "setfocus.h"
#include "setmmap.h"
#include "setmode.h"
#include "ungrdev.h"
#include "ungrdevb.h"
#include "ungrdevk.h"
#include "xiallowev.h"
#include "xiselectev.h"
#include "xigrabdev.h"
#include "xipassivegrab.h"
#include "xisetdevfocus.h"
#include "xiproperty.h"
#include "xichangecursor.h"
#include "xichangehierarchy.h"
#include "xigetclientpointer.h"
#include "xiquerydevice.h"
#include "xiquerypointer.h"
#include "xiqueryversion.h"
#include "xisetclientpointer.h"
#include "xiwarppointer.h"
#include "xibarriers.h"

/* Masks for XI events have to be aligned with core event (partially anyway).
 * If DeviceButtonMotionMask is != ButtonMotionMask, event delivery
 * breaks down. The device needs the dev->button->motionMask. If DBMM is
 * the same as BMM, we can ensure that both core and device events can be
 * delivered, without the need for extra structures in the DeviceIntRec. */
const Mask DeviceProximityMask = (1L << 4);
const Mask DeviceStateNotifyMask = (1L << 5);
const Mask DevicePointerMotionHintMask = PointerMotionHintMask;
const Mask DeviceButton1MotionMask = Button1MotionMask;
const Mask DeviceButton2MotionMask = Button2MotionMask;
const Mask DeviceButton3MotionMask = Button3MotionMask;
const Mask DeviceButton4MotionMask = Button4MotionMask;
const Mask DeviceButton5MotionMask = Button5MotionMask;
const Mask DeviceButtonMotionMask = ButtonMotionMask;
const Mask DeviceFocusChangeMask = (1L << 14);
const Mask DeviceMappingNotifyMask = (1L << 15);
const Mask ChangeDeviceNotifyMask = (1L << 16);
const Mask DeviceButtonGrabMask = (1L << 17);
const Mask DeviceOwnerGrabButtonMask = (1L << 17);
const Mask DevicePresenceNotifyMask = (1L << 18);
const Mask DevicePropertyNotifyMask = (1L << 19);
const Mask XIAllMasks = (1L << 20) - 1;


static struct dev_type {
    Atom type;
    const char *name;
} dev_type[] = {
    {0, XI_KEYBOARD},
    {0, XI_MOUSE},
    {0, XI_TABLET},
    {0, XI_TOUCHSCREEN},
    {0, XI_TOUCHPAD},
    {0, XI_BARCODE},
    {0, XI_BUTTONBOX},
    {0, XI_KNOB_BOX},
    {0, XI_ONE_KNOB},
    {0, XI_NINE_KNOB},
    {0, XI_TRACKBALL},
    {0, XI_QUADRATURE},
    {0, XI_ID_MODULE},
    {0, XI_SPACEBALL},
    {0, XI_DATAGLOVE},
    {0, XI_EYETRACKER},
    {0, XI_CURSORKEYS},
    {0, XI_FOOTMOUSE}
};

/* CARD8 event_base[numInputClasses]; - moved to context */
// EventInfo moved to context->EventInfo

static DeviceIntRec xi_all_devices;
static DeviceIntRec xi_all_master_devices;

/**
 * Dispatch vector. Functions defined in here will be called when the matching
 * request arrives.
 */
static int (*ProcIVector[]) (ClientPtr) = {
    NULL,                       /*  0 */
        ProcXGetExtensionVersion,       /*  1 */
        ProcXListInputDevices,  /*  2 */
        ProcXOpenDevice,        /*  3 */
        ProcXCloseDevice,       /*  4 */
        ProcXSetDeviceMode,     /*  5 */
        ProcXSelectExtensionEvent,      /*  6 */
        ProcXGetSelectedExtensionEvents,        /*  7 */
        ProcXChangeDeviceDontPropagateList,     /*  8 */
        ProcXGetDeviceDontPropagateList,        /*  9 */
        ProcXGetDeviceMotionEvents,     /* 10 */
        ProcXChangeKeyboardDevice,      /* 11 */
        ProcXChangePointerDevice,       /* 12 */
        ProcXGrabDevice,        /* 13 */
        ProcXUngrabDevice,      /* 14 */
        ProcXGrabDeviceKey,     /* 15 */
        ProcXUngrabDeviceKey,   /* 16 */
        ProcXGrabDeviceButton,  /* 17 */
        ProcXUngrabDeviceButton,        /* 18 */
        ProcXAllowDeviceEvents, /* 19 */
        ProcXGetDeviceFocus,    /* 20 */
        ProcXSetDeviceFocus,    /* 21 */
        ProcXGetFeedbackControl,        /* 22 */
        ProcXChangeFeedbackControl,     /* 23 */
        ProcXGetDeviceKeyMapping,       /* 24 */
        ProcXChangeDeviceKeyMapping,    /* 25 */
        ProcXGetDeviceModifierMapping,  /* 26 */
        ProcXSetDeviceModifierMapping,  /* 27 */
        ProcXGetDeviceButtonMapping,    /* 28 */
        ProcXSetDeviceButtonMapping,    /* 29 */
        ProcXQueryDeviceState,  /* 30 */
        ProcXSendExtensionEvent,        /* 31 */
        ProcXDeviceBell,        /* 32 */
        ProcXSetDeviceValuators,        /* 33 */
        ProcXGetDeviceControl,  /* 34 */
        ProcXChangeDeviceControl,       /* 35 */
        /* XI 1.5 */
        ProcXListDeviceProperties,      /* 36 */
        ProcXChangeDeviceProperty,      /* 37 */
        ProcXDeleteDeviceProperty,      /* 38 */
        ProcXGetDeviceProperty, /* 39 */
        /* XI 2 */
        ProcXIQueryPointer,     /* 40 */
        ProcXIWarpPointer,      /* 41 */
        ProcXIChangeCursor,     /* 42 */
        ProcXIChangeHierarchy,  /* 43 */
        ProcXISetClientPointer, /* 44 */
        ProcXIGetClientPointer, /* 45 */
        ProcXISelectEvents,     /* 46 */
        ProcXIQueryVersion,     /* 47 */
        ProcXIQueryDevice,      /* 48 */
        ProcXISetFocus,         /* 49 */
        ProcXIGetFocus,         /* 50 */
        ProcXIGrabDevice,       /* 51 */
        ProcXIUngrabDevice,     /* 52 */
        ProcXIAllowEvents,      /* 53 */
        ProcXIPassiveGrabDevice,        /* 54 */
        ProcXIPassiveUngrabDevice,      /* 55 */
        ProcXIListProperties,   /* 56 */
        ProcXIChangeProperty,   /* 57 */
        ProcXIDeleteProperty,   /* 58 */
        ProcXIGetProperty,      /* 59 */
        ProcXIGetSelectedEvents, /* 60 */
        ProcXIBarrierReleasePointer /* 61 */
};

/* For swapped context->clients */
static int (*SProcIVector[]) (ClientPtr) = {
    NULL,                       /*  0 */
        SProcXGetExtensionVersion,      /*  1 */
        SProcXListInputDevices, /*  2 */
        SProcXOpenDevice,       /*  3 */
        SProcXCloseDevice,      /*  4 */
        SProcXSetDeviceMode,    /*  5 */
        SProcXSelectExtensionEvent,     /*  6 */
        SProcXGetSelectedExtensionEvents,       /*  7 */
        SProcXChangeDeviceDontPropagateList,    /*  8 */
        SProcXGetDeviceDontPropagateList,       /*  9 */
        SProcXGetDeviceMotionEvents,    /* 10 */
        SProcXChangeKeyboardDevice,     /* 11 */
        SProcXChangePointerDevice,      /* 12 */
        SProcXGrabDevice,       /* 13 */
        SProcXUngrabDevice,     /* 14 */
        SProcXGrabDeviceKey,    /* 15 */
        SProcXUngrabDeviceKey,  /* 16 */
        SProcXGrabDeviceButton, /* 17 */
        SProcXUngrabDeviceButton,       /* 18 */
        SProcXAllowDeviceEvents,        /* 19 */
        SProcXGetDeviceFocus,   /* 20 */
        SProcXSetDeviceFocus,   /* 21 */
        SProcXGetFeedbackControl,       /* 22 */
        SProcXChangeFeedbackControl,    /* 23 */
        SProcXGetDeviceKeyMapping,      /* 24 */
        SProcXChangeDeviceKeyMapping,   /* 25 */
        SProcXGetDeviceModifierMapping, /* 26 */
        SProcXSetDeviceModifierMapping, /* 27 */
        SProcXGetDeviceButtonMapping,   /* 28 */
        SProcXSetDeviceButtonMapping,   /* 29 */
        SProcXQueryDeviceState, /* 30 */
        SProcXSendExtensionEvent,       /* 31 */
        SProcXDeviceBell,       /* 32 */
        SProcXSetDeviceValuators,       /* 33 */
        SProcXGetDeviceControl, /* 34 */
        SProcXChangeDeviceControl,      /* 35 */
        SProcXListDeviceProperties,     /* 36 */
        SProcXChangeDeviceProperty,     /* 37 */
        SProcXDeleteDeviceProperty,     /* 38 */
        SProcXGetDeviceProperty,        /* 39 */
        SProcXIQueryPointer,    /* 40 */
        SProcXIWarpPointer,     /* 41 */
        SProcXIChangeCursor,    /* 42 */
        SProcXIChangeHierarchy, /* 43 */
        SProcXISetClientPointer,        /* 44 */
        SProcXIGetClientPointer,        /* 45 */
        SProcXISelectEvents,    /* 46 */
        SProcXIQueryVersion,    /* 47 */
        SProcXIQueryDevice,     /* 48 */
        SProcXISetFocus,        /* 49 */
        SProcXIGetFocus,        /* 50 */
        SProcXIGrabDevice,      /* 51 */
        SProcXIUngrabDevice,    /* 52 */
        SProcXIAllowEvents,     /* 53 */
        SProcXIPassiveGrabDevice,       /* 54 */
        SProcXIPassiveUngrabDevice,     /* 55 */
        SProcXIListProperties,  /* 56 */
        SProcXIChangeProperty,  /* 57 */
        SProcXIDeleteProperty,  /* 58 */
        SProcXIGetProperty,     /* 59 */
        SProcXIGetSelectedEvents,       /* 60 */
        SProcXIBarrierReleasePointer /* 61 */
};

/*****************************************************************
 *
 * Globals referenced elsewhere in the server.
 *
 */

/* Static variables for event indices - needed by callback functions that cannot access context */
static int s_DeviceValuator = 0;
static int s_DeviceKeyPress = 0;
static int s_DeviceKeyRelease = 0;
static int s_DeviceButtonPress = 0;
static int s_DeviceButtonRelease = 0;
static int s_DeviceMotionNotify = 0;
static int s_DeviceFocusIn = 0;
static int s_DeviceFocusOut = 0;
static int s_ProximityIn = 0;
static int s_ProximityOut = 0;
static int s_DeviceStateNotify = 0;
static int s_DeviceMappingNotify = 0;
static int s_ChangeDeviceNotify = 0;
static int s_DeviceKeyStateNotify = 0;
static int s_DeviceButtonStateNotify = 0;
static int s_DevicePresenceNotify = 0;
static int s_DevicePropertyNotify = 0;

static int BadEvent = 1;

/*****************************************************************
 *
 * Externs defined elsewhere in the X server.
 *
 */

/* extern XExtensionVersion XIVersion; */

/*****************************************************************
 *
 * Versioning support
 *
 */

/* Moved to XephyrContext:
DevPrivateKeyRec XIClientPrivateKeyRec;
*/

/*****************************************************************
 *
 * Declarations of local routines.
 *
 */

/*************************************************************************
 *
 * ProcIDispatch - main dispatch routine for requests to this extension.
 * This routine is used if server and client have the same byte ordering.
 *
 */

static int
ProcIDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= ARRAY_SIZE(ProcIVector) || !ProcIVector[stuff->data])
        return BadRequest;

    UpdateCurrentTimeIf(client->context);
    return (*ProcIVector[stuff->data]) (client);
}

/*******************************************************************************
 *
 * SProcXDispatch
 *
 * Main swapped dispatch routine for requests to this extension.
 * This routine is used if server and client do not have the same byte ordering.
 *
 */

static int _X_COLD
SProcIDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= ARRAY_SIZE(SProcIVector) || !SProcIVector[stuff->data])
        return BadRequest;

    UpdateCurrentTimeIf(client->context);
    return (*SProcIVector[stuff->data]) (client);
}

/**********************************************************************
 *
 * SReplyIDispatch
 * Swap any replies defined in this extension.
 *
 */

static void _X_COLD
SReplyIDispatch(ClientPtr client, int len, xGrabDeviceReply * rep)
{
    /* All we look at is the type field */
    /* This is common to all replies    */
    if (rep->RepType == X_GetExtensionVersion)
        SRepXGetExtensionVersion(client, len,
                                 (xGetExtensionVersionReply *) rep);
    else if (rep->RepType == X_ListInputDevices)
        SRepXListInputDevices(client, len, (xListInputDevicesReply *) rep);
    else if (rep->RepType == X_OpenDevice)
        SRepXOpenDevice(client, len, (xOpenDeviceReply *) rep);
    else if (rep->RepType == X_SetDeviceMode)
        SRepXSetDeviceMode(client, len, (xSetDeviceModeReply *) rep);
    else if (rep->RepType == X_GetSelectedExtensionEvents)
        SRepXGetSelectedExtensionEvents(client, len,
                                        (xGetSelectedExtensionEventsReply *)
                                        rep);
    else if (rep->RepType == X_GetDeviceDontPropagateList)
        SRepXGetDeviceDontPropagateList(client, len,
                                        (xGetDeviceDontPropagateListReply *)
                                        rep);
    else if (rep->RepType == X_GetDeviceMotionEvents)
        SRepXGetDeviceMotionEvents(client, len,
                                   (xGetDeviceMotionEventsReply *) rep);
    else if (rep->RepType == X_GrabDevice)
        SRepXGrabDevice(client, len, (xGrabDeviceReply *) rep);
    else if (rep->RepType == X_GetDeviceFocus)
        SRepXGetDeviceFocus(client, len, (xGetDeviceFocusReply *) rep);
    else if (rep->RepType == X_GetFeedbackControl)
        SRepXGetFeedbackControl(client, len, (xGetFeedbackControlReply *) rep);
    else if (rep->RepType == X_GetDeviceKeyMapping)
        SRepXGetDeviceKeyMapping(client, len,
                                 (xGetDeviceKeyMappingReply *) rep);
    else if (rep->RepType == X_GetDeviceModifierMapping)
        SRepXGetDeviceModifierMapping(client, len,
                                      (xGetDeviceModifierMappingReply *) rep);
    else if (rep->RepType == X_SetDeviceModifierMapping)
        SRepXSetDeviceModifierMapping(client, len,
                                      (xSetDeviceModifierMappingReply *) rep);
    else if (rep->RepType == X_GetDeviceButtonMapping)
        SRepXGetDeviceButtonMapping(client, len,
                                    (xGetDeviceButtonMappingReply *) rep);
    else if (rep->RepType == X_SetDeviceButtonMapping)
        SRepXSetDeviceButtonMapping(client, len,
                                    (xSetDeviceButtonMappingReply *) rep);
    else if (rep->RepType == X_QueryDeviceState)
        SRepXQueryDeviceState(client, len, (xQueryDeviceStateReply *) rep);
    else if (rep->RepType == X_SetDeviceValuators)
        SRepXSetDeviceValuators(client, len, (xSetDeviceValuatorsReply *) rep);
    else if (rep->RepType == X_GetDeviceControl)
        SRepXGetDeviceControl(client, len, (xGetDeviceControlReply *) rep);
    else if (rep->RepType == X_ChangeDeviceControl)
        SRepXChangeDeviceControl(client, len,
                                 (xChangeDeviceControlReply *) rep);
    else if (rep->RepType == X_ListDeviceProperties)
        SRepXListDeviceProperties(client, len,
                                  (xListDevicePropertiesReply *) rep);
    else if (rep->RepType == X_GetDeviceProperty)
        SRepXGetDeviceProperty(client, len, (xGetDevicePropertyReply *) rep);
    else if (rep->RepType == X_XIQueryPointer)
        SRepXIQueryPointer(client, len, (xXIQueryPointerReply *) rep);
    else if (rep->RepType == X_XIGetClientPointer)
        SRepXIGetClientPointer(client, len, (xXIGetClientPointerReply *) rep);
    else if (rep->RepType == X_XIQueryVersion)
        SRepXIQueryVersion(client, len, (xXIQueryVersionReply *) rep);
    else if (rep->RepType == X_XIQueryDevice)
        SRepXIQueryDevice(client, len, (xXIQueryDeviceReply *) rep);
    else if (rep->RepType == X_XIGrabDevice)
        SRepXIGrabDevice(client, len, (xXIGrabDeviceReply *) rep);
    else if (rep->RepType == X_XIPassiveGrabDevice)
        SRepXIPassiveGrabDevice(client, len, (xXIPassiveGrabDeviceReply *) rep);
    else if (rep->RepType == X_XIListProperties)
        SRepXIListProperties(client, len, (xXIListPropertiesReply *) rep);
    else if (rep->RepType == X_XIGetProperty)
        SRepXIGetProperty(client, len, (xXIGetPropertyReply *) rep);
    else if (rep->RepType == X_XIGetSelectedEvents)
        SRepXIGetSelectedEvents(client, len, (xXIGetSelectedEventsReply *) rep);
    else if (rep->RepType == X_XIGetFocus)
        SRepXIGetFocus(client, len, (xXIGetFocusReply *) rep);
    else {
        fprintf(stderr, "XINPUT confused sending swapped reply\n");
        abort();
    }
}

/************************************************************************
 *
 * This function swaps the context->DeviceValuator event.
 *
 */

static void
SEventDeviceValuator(deviceValuator * from, deviceValuator * to)
{
    int i;
    INT32 *ip;

    *to = *from;
    swaps(&to->sequenceNumber);
    swaps(&to->device_state);
    ip = &to->valuator0;
    for (i = 0; i < 6; i++) {
        swapl(ip + i);
    }
}

static void
SEventFocus(deviceFocus * from, deviceFocus * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    swapl(&to->window);
}

static void
SDeviceStateNotifyEvent(deviceStateNotify * from, deviceStateNotify * to)
{
    int i;
    INT32 *ip;

    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    ip = &to->valuator0;
    for (i = 0; i < 3; i++) {
        swapl(ip + i);
    }
}

static void
SDeviceKeyStateNotifyEvent(deviceKeyStateNotify * from,
                           deviceKeyStateNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
}

static void
SDeviceButtonStateNotifyEvent(deviceButtonStateNotify * from,
                              deviceButtonStateNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
}

static void
SChangeDeviceNotifyEvent(changeDeviceNotify * from, changeDeviceNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
}

static void
SDeviceMappingNotifyEvent(deviceMappingNotify * from, deviceMappingNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
}

static void
SDevicePresenceNotifyEvent(devicePresenceNotify * from,
                           devicePresenceNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    swaps(&to->control);
}

static void
SDevicePropertyNotifyEvent(devicePropertyNotify * from,
                           devicePropertyNotify * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->time);
    swapl(&to->atom);
}

static void
SDeviceLeaveNotifyEvent(xXILeaveEvent * from, xXILeaveEvent * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);
    swaps(&to->sourceid);
    swaps(&to->buttons_len);
    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
}

static void
SDeviceChangedEvent(xXIDeviceChangedEvent * from, xXIDeviceChangedEvent * to)
{
    int i, j;
    xXIAnyInfo *any;

    *to = *from;
    memcpy(&to[1], &from[1], from->length * 4);

    any = (xXIAnyInfo *) &to[1];
    for (i = 0; i < to->num_classes; i++) {
        int length = any->length;

        switch (any->type) {
        case KeyClass:
        {
            xXIKeyInfo *ki = (xXIKeyInfo *) any;
            uint32_t *key = (uint32_t *) &ki[1];

            for (j = 0; j < ki->num_keycodes; j++, key++)
                swapl(key);
            swaps(&ki->num_keycodes);
        }
            break;
        case ButtonClass:
        {
            xXIButtonInfo *bi = (xXIButtonInfo *) any;
            Atom *labels = (Atom *) ((char *) bi + sizeof(xXIButtonInfo) +
                                     pad_to_int32(bits_to_bytes
                                                  (bi->num_buttons)));
            for (j = 0; j < bi->num_buttons; j++)
                swapl(&labels[j]);
            swaps(&bi->num_buttons);
        }
            break;
        case ValuatorClass:
        {
            xXIValuatorInfo *ai = (xXIValuatorInfo *) any;

            swapl(&ai->label);
            swapl(&ai->min.integral);
            swapl(&ai->min.frac);
            swapl(&ai->max.integral);
            swapl(&ai->max.frac);
            swapl(&ai->resolution);
            swaps(&ai->number);
        }
            break;
        }

        swaps(&any->type);
        swaps(&any->length);
        swaps(&any->sourceid);

        any = (xXIAnyInfo *) ((char *) any + length * 4);
    }

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swaps(&to->num_classes);
    swaps(&to->sourceid);

}

static void
SDeviceEvent(xXIDeviceEvent * from, xXIDeviceEvent * to)
{
    int i;
    char *ptr;
    char *vmask;

    memcpy(to, from, sizeof(xEvent) + from->length * 4);

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);
    swaps(&to->buttons_len);
    swaps(&to->valuators_len);
    swaps(&to->sourceid);
    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
    swapl(&to->mods.effective_mods);
    swapl(&to->flags);

    ptr = (char *) (&to[1]);
    ptr += from->buttons_len * 4;
    vmask = ptr;                /* valuator mask */
    ptr += from->valuators_len * 4;
    for (i = 0; i < from->valuators_len * 32; i++) {
        if (BitIsOn(vmask, i)) {
            swapl(((uint32_t *) ptr));
            ptr += 4;
            swapl(((uint32_t *) ptr));
            ptr += 4;
        }
    }
}

static void
SDeviceHierarchyEvent(xXIHierarchyEvent * from, xXIHierarchyEvent * to)
{
    int i;
    xXIHierarchyInfo *info;

    *to = *from;
    memcpy(&to[1], &from[1], from->length * 4);
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->flags);
    swaps(&to->num_info);

    info = (xXIHierarchyInfo *) &to[1];
    for (i = 0; i < from->num_info; i++) {
        swaps(&info->deviceid);
        swaps(&info->attachment);
        info++;
    }
}

static void
SXIPropertyEvent(xXIPropertyEvent * from, xXIPropertyEvent * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->property);
}

static void
SRawEvent(xXIRawEvent * from, xXIRawEvent * to)
{
    int i;
    FP3232 *values;
    unsigned char *mask;

    memcpy(to, from, sizeof(xEvent) + from->length * 4);

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);

    mask = (unsigned char *) &to[1];
    values = (FP3232 *) (mask + from->valuators_len * 4);

    for (i = 0; i < from->valuators_len * 4 * 8; i++) {
        if (BitIsOn(mask, i)) {
            /* for each bit set there are two FP3232 values on the wire, in
             * the order abcABC for data and data_raw. Here we swap as if
             * they were in aAbBcC order because it's easier and really
             * doesn't matter.
             */
            swapl(&values->integral);
            swapl(&values->frac);
            values++;
            swapl(&values->integral);
            swapl(&values->frac);
            values++;
        }
    }

    swaps(&to->valuators_len);
}

static void
STouchOwnershipEvent(xXITouchOwnershipEvent * from, xXITouchOwnershipEvent * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swaps(&to->sourceid);
    swapl(&to->touchid);
    swapl(&to->flags);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
}

static void
SBarrierEvent(xXIBarrierEvent * from,
              xXIBarrierEvent * to) {

    *to = *from;

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swapl(&to->time);
    swaps(&to->deviceid);
    swaps(&to->sourceid);
    swapl(&to->event);
    swapl(&to->root);
    swapl(&to->root_x);
    swapl(&to->root_y);

    swapl(&to->dx.integral);
    swapl(&to->dx.frac);
    swapl(&to->dy.integral);
    swapl(&to->dy.frac);
    swapl(&to->dtime);
    swapl(&to->barrier);
    swapl(&to->eventid);
}

static void
SGesturePinchEvent(xXIGesturePinchEvent* from,
                   xXIGesturePinchEvent* to)
{
    *to = *from;

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);

    swapl(&to->delta_x);
    swapl(&to->delta_y);
    swapl(&to->delta_unaccel_x);
    swapl(&to->delta_unaccel_y);
    swapl(&to->scale);
    swapl(&to->delta_angle);
    swaps(&to->sourceid);

    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
    swapl(&to->mods.effective_mods);
    swapl(&to->flags);
}

static void
SGestureSwipeEvent(xXIGestureSwipeEvent* from,
                   xXIGestureSwipeEvent* to)
{
    *to = *from;

    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    swaps(&to->deviceid);
    swapl(&to->time);
    swapl(&to->detail);
    swapl(&to->root);
    swapl(&to->event);
    swapl(&to->child);
    swapl(&to->root_x);
    swapl(&to->root_y);
    swapl(&to->event_x);
    swapl(&to->event_y);

    swapl(&to->delta_x);
    swapl(&to->delta_y);
    swapl(&to->delta_unaccel_x);
    swapl(&to->delta_unaccel_y);
    swaps(&to->sourceid);

    swapl(&to->mods.base_mods);
    swapl(&to->mods.latched_mods);
    swapl(&to->mods.locked_mods);
    swapl(&to->mods.effective_mods);
    swapl(&to->flags);
}

/** Event swapping function for XI2 events. */
void _X_COLD
XI2EventSwap(xGenericEvent *from, xGenericEvent *to)
{
    switch (from->evtype) {
    case XI_Enter:
    case XI_Leave:
    case XI_FocusIn:
    case XI_FocusOut:
        SDeviceLeaveNotifyEvent((xXILeaveEvent *) from, (xXILeaveEvent *) to);
        break;
    case XI_DeviceChanged:
        SDeviceChangedEvent((xXIDeviceChangedEvent *) from,
                            (xXIDeviceChangedEvent *) to);
        break;
    case XI_HierarchyChanged:
        SDeviceHierarchyEvent((xXIHierarchyEvent *) from,
                              (xXIHierarchyEvent *) to);
        break;
    case XI_PropertyEvent:
        SXIPropertyEvent((xXIPropertyEvent *) from, (xXIPropertyEvent *) to);
        break;
    case XI_Motion:
    case XI_KeyPress:
    case XI_KeyRelease:
    case XI_ButtonPress:
    case XI_ButtonRelease:
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd:
        SDeviceEvent((xXIDeviceEvent *) from, (xXIDeviceEvent *) to);
        break;
    case XI_TouchOwnership:
        STouchOwnershipEvent((xXITouchOwnershipEvent *) from,
                             (xXITouchOwnershipEvent *) to);
        break;
    case XI_RawMotion:
    case XI_RawKeyPress:
    case XI_RawKeyRelease:
    case XI_RawButtonPress:
    case XI_RawButtonRelease:
    case XI_RawTouchBegin:
    case XI_RawTouchUpdate:
    case XI_RawTouchEnd:
        SRawEvent((xXIRawEvent *) from, (xXIRawEvent *) to);
        break;
    case XI_BarrierHit:
    case XI_BarrierLeave:
        SBarrierEvent((xXIBarrierEvent *) from,
                      (xXIBarrierEvent *) to);
        break;
    case XI_GesturePinchBegin:
    case XI_GesturePinchUpdate:
    case XI_GesturePinchEnd:
        SGesturePinchEvent((xXIGesturePinchEvent*) from,
                           (xXIGesturePinchEvent*) to);
        break;
    case XI_GestureSwipeBegin:
    case XI_GestureSwipeUpdate:
    case XI_GestureSwipeEnd:
        SGestureSwipeEvent((xXIGestureSwipeEvent*) from,
                           (xXIGestureSwipeEvent*) to);
        break;
    default:
        fprintf(stderr, "[Xi] Unknown event type to swap. This is a bug.\n");
        break;
    }
}

/**************************************************************************
 *
 * Record an event mask where there is no unique corresponding event type.
 * We can't call SetMaskForEvent, since that would clobber the existing
 * mask for that event.  MotionHint and ButtonMotion are examples.
 *
 * Since extension event types will never be less than 64, we can use
 * 0-63 in the EventInfo array as the "type" to be used to look up this
 * mask.  This means that the corresponding macros such as
 * DevicePointerMotionHint must have access to the same constants.
 *
 */

static void
SetEventInfo(Mask mask, int constant, XephyrContext* context)
{
    context->EventInfo[context->ExtEventIndex].mask = mask;
    context->EventInfo[context->ExtEventIndex++].type = constant;
}

/**************************************************************************
 *
 * Assign the specified mask to the specified event.
 *
 */

static void
SetMaskForExtEvent(Mask mask, int event, XephyrContext* context)
{
    int i;

    context->EventInfo[context->ExtEventIndex].mask = mask;
    context->EventInfo[context->ExtEventIndex++].type = event;

    if ((event < LASTEvent) || (event >= 128))
        FatalError("MaskForExtensionEvent: bogus event number", context);

    for (i = 0; i < MAXDEVICES; i++)
        SetMaskForEvent(i, mask, event, context);
}

/************************************************************************
 *
 * This function sets up extension event types and masks.
 *
 */

static void
FixExtensionEvents(ExtensionEntry * extEntry, XephyrContext* context)
{
    context->DeviceValuator = extEntry->eventBase;
    s_DeviceValuator = context->DeviceValuator;
    
    context->DeviceKeyPress = context->DeviceValuator + 1;
    s_DeviceKeyPress = context->DeviceKeyPress;
    
    context->DeviceKeyRelease = context->DeviceKeyPress + 1;
    s_DeviceKeyRelease = context->DeviceKeyRelease;
    
    context->DeviceButtonPress = context->DeviceKeyRelease + 1;
    s_DeviceButtonPress = context->DeviceButtonPress;
    
    context->DeviceButtonRelease = context->DeviceButtonPress + 1;
    s_DeviceButtonRelease = context->DeviceButtonRelease;
    
    context->DeviceMotionNotify = context->DeviceButtonRelease + 1;
    s_DeviceMotionNotify = context->DeviceMotionNotify;
    
    context->DeviceFocusIn = context->DeviceMotionNotify + 1;
    s_DeviceFocusIn = context->DeviceFocusIn;
    
    context->DeviceFocusOut = context->DeviceFocusIn + 1;
    s_DeviceFocusOut = context->DeviceFocusOut;
    
    context->ProximityIn = context->DeviceFocusOut + 1;
    s_ProximityIn = context->ProximityIn;
    
    context->ProximityOut = context->ProximityIn + 1;
    s_ProximityOut = context->ProximityOut;
    
    context->DeviceStateNotify = context->ProximityOut + 1;
    s_DeviceStateNotify = context->DeviceStateNotify;
    
    context->DeviceMappingNotify = context->DeviceStateNotify + 1;
    s_DeviceMappingNotify = context->DeviceMappingNotify;
    
    context->ChangeDeviceNotify = context->DeviceMappingNotify + 1;
    s_ChangeDeviceNotify = context->ChangeDeviceNotify;
    
    context->DeviceKeyStateNotify = context->ChangeDeviceNotify + 1;
    s_DeviceKeyStateNotify = context->DeviceKeyStateNotify;
    
    context->DeviceButtonStateNotify = context->DeviceKeyStateNotify + 1;
    s_DeviceButtonStateNotify = context->DeviceButtonStateNotify;
    
    context->DevicePresenceNotify = context->DeviceButtonStateNotify + 1;
    s_DevicePresenceNotify = context->DevicePresenceNotify;
    
    context->DevicePropertyNotify = context->DevicePresenceNotify + 1;
    s_DevicePropertyNotify = context->DevicePropertyNotify;

    context->event_base[KeyClass] = context->DeviceKeyPress;
    context->event_base[ButtonClass] = context->DeviceButtonPress;
    context->event_base[ValuatorClass] = context->DeviceMotionNotify;
    context->event_base[ProximityClass] = context->ProximityIn;
    context->event_base[FocusClass] = context->DeviceFocusIn;
    context->event_base[OtherClass] = context->DeviceStateNotify;

    context->BadDevice += extEntry->errorBase;
    BadEvent += extEntry->errorBase;
    context->BadMode += extEntry->errorBase;
    context->DeviceBusy += extEntry->errorBase;
    context->BadClass += extEntry->errorBase;

    SetMaskForExtEvent(KeyPressMask, context->DeviceKeyPress, context);
    SetCriticalEvent(context->DeviceKeyPress, context);

    SetMaskForExtEvent(KeyReleaseMask, context->DeviceKeyRelease, context);
    SetCriticalEvent(context->DeviceKeyRelease, context);

    SetMaskForExtEvent(ButtonPressMask, context->DeviceButtonPress, context);
    SetCriticalEvent(context->DeviceButtonPress, context);

    SetMaskForExtEvent(ButtonReleaseMask, context->DeviceButtonRelease, context);
    SetCriticalEvent(context->DeviceButtonRelease, context);

    SetMaskForExtEvent(DeviceProximityMask, context->ProximityIn, context);
    SetMaskForExtEvent(DeviceProximityMask, context->ProximityOut, context);

    SetMaskForExtEvent(DeviceStateNotifyMask, context->DeviceStateNotify, context);

    SetMaskForExtEvent(PointerMotionMask, context->DeviceMotionNotify, context);
    SetCriticalEvent(context->DeviceMotionNotify, context);

    SetEventInfo(DevicePointerMotionHintMask, _devicePointerMotionHint, context);
    SetEventInfo(DeviceButton1MotionMask, _deviceButton1Motion, context);
    SetEventInfo(DeviceButton2MotionMask, _deviceButton2Motion, context);
    SetEventInfo(DeviceButton3MotionMask, _deviceButton3Motion, context);
    SetEventInfo(DeviceButton4MotionMask, _deviceButton4Motion, context);
    SetEventInfo(DeviceButton5MotionMask, _deviceButton5Motion, context);
    SetEventInfo(DeviceButtonMotionMask, _deviceButtonMotion, context);

    SetMaskForExtEvent(DeviceFocusChangeMask, context->DeviceFocusIn, context);
    SetMaskForExtEvent(DeviceFocusChangeMask, context->DeviceFocusOut, context);

    SetMaskForExtEvent(DeviceMappingNotifyMask, context->DeviceMappingNotify, context);
    SetMaskForExtEvent(ChangeDeviceNotifyMask, context->ChangeDeviceNotify, context);

    SetEventInfo(DeviceButtonGrabMask, _deviceButtonGrab, context);
    SetEventInfo(DeviceOwnerGrabButtonMask, _deviceOwnerGrabButton, context);
    SetEventInfo(DevicePresenceNotifyMask, _devicePresence, context);
    SetMaskForExtEvent(DevicePropertyNotifyMask, context->DevicePropertyNotify, context);

    SetEventInfo(0, _noExtensionEvent, context);
}

/************************************************************************
 *
 * This function restores extension event types and masks to their
 * initial state.
 *
 */

static void
RestoreExtensionEvents(XephyrContext* context)
{
    int i, j;

    context->IReqCode = 0;
    context->IEventBase = 0;

    for (i = 0; i < context->ExtEventIndex - 1; i++) {
        if ((context->EventInfo[i].type >= LASTEvent) && (context->EventInfo[i].type < 128)) {
            for (j = 0; j < MAXDEVICES; j++)
                SetMaskForEvent(j, 0, context->EventInfo[i].type, context);
        }
        context->EventInfo[i].mask = 0;
        context->EventInfo[i].type = 0;
    }
    context->ExtEventIndex = 0;
    context->DeviceValuator = 0;
    context->DeviceKeyPress = 1;
    context->DeviceKeyRelease = 2;
    context->DeviceButtonPress = 3;
    context->DeviceButtonRelease = 4;
    context->DeviceMotionNotify = 5;
    context->DeviceFocusIn = 6;
    context->DeviceFocusOut = 7;
    context->ProximityIn = 8;
    context->ProximityOut = 9;
    context->DeviceStateNotify = 10;
    context->DeviceMappingNotify = 11;
    context->ChangeDeviceNotify = 12;
    context->DeviceKeyStateNotify = 13;
    context->DeviceButtonStateNotify = 13;
    context->DevicePresenceNotify = 14;
    context->DevicePropertyNotify = 15;

    context->BadDevice = 0;
    BadEvent = 1;
    context->BadMode = 2;
    context->DeviceBusy = 3;
    context->BadClass = 4;

}

/***********************************************************************
 *
 * IResetProc.
 * Remove reply-swapping routine.
 * Remove event-swapping routine.
 *
 */

static void
IResetProc(ExtensionEntry * extEntry)
{
    ReplySwapVector[extEntry->base] = ReplyNotSwappd;
    /* Reset event swap vectors based on event base */
    int eventBase = extEntry->eventBase;
    EventSwapVector[eventBase] = NotImplemented;      /* DeviceValuator */
    EventSwapVector[eventBase + 1] = NotImplemented;  /* context->DeviceKeyPress */
    EventSwapVector[eventBase + 2] = NotImplemented;  /* context->DeviceKeyRelease */
    EventSwapVector[eventBase + 3] = NotImplemented;  /* context->DeviceButtonPress */
    EventSwapVector[eventBase + 4] = NotImplemented;  /* context->DeviceButtonRelease */
    EventSwapVector[eventBase + 5] = NotImplemented;  /* context->DeviceMotionNotify */
    EventSwapVector[eventBase + 6] = NotImplemented;  /* context->DeviceFocusIn */
    EventSwapVector[eventBase + 7] = NotImplemented;  /* context->DeviceFocusOut */
    EventSwapVector[eventBase + 8] = NotImplemented;  /* context->ProximityIn */
    EventSwapVector[eventBase + 9] = NotImplemented;  /* context->ProximityOut */
    EventSwapVector[eventBase + 10] = NotImplemented; /* context->DeviceStateNotify */
    EventSwapVector[eventBase + 11] = NotImplemented; /* context->DeviceMappingNotify */
    EventSwapVector[eventBase + 12] = NotImplemented; /* ChangeDeviceNotify */
    EventSwapVector[eventBase + 13] = NotImplemented; /* context->DeviceKeyStateNotify */
    EventSwapVector[eventBase + 14] = NotImplemented; /* context->DeviceButtonStateNotify */
    EventSwapVector[eventBase + 15] = NotImplemented; /* context->DevicePresenceNotify */
    EventSwapVector[eventBase + 16] = NotImplemented; /* context->DevicePropertyNotify */
    /* Cannot call RestoreExtensionEvents without context access */
    /* This is called when extension is unloaded, which typically happens at server shutdown */

    free(xi_all_devices.name);
    free(xi_all_master_devices.name);

    /* TODO: Need to get context for XIBarrierReset - for now skip */
    /* XIBarrierReset(context); */
}

/***********************************************************************
 *
 * Assign an id and type to an input device.
 *
 */

void
AssignTypeAndName(DeviceIntPtr dev, Atom type, const char *name)
{
    dev->xinput_type = type;
    dev->name = strdup(name);
}

/***********************************************************************
 *
 * Make device type atoms.
 *
 */

static void
MakeDeviceTypeAtoms(void)
{
    int i;

    for (i = 0; i < NUMTYPES; i++)
        dev_type[i].type =
            MakeAtom(dev_type[i].name, strlen(dev_type[i].name), 1);
}

/*****************************************************************************
 *
 *	SEventIDispatch
 *
 *	Swap any events defined in this extension.
 */
#define DO_SWAP(func,type) func ((type *)from, (type *)to)

static void _X_COLD
SEventIDispatch(xEvent *from, xEvent *to)
{
    int type = from->u.u.type & 0177;

    if (type == s_DeviceValuator)
        DO_SWAP(SEventDeviceValuator, deviceValuator);
    else if (type == s_DeviceKeyPress) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == s_DeviceKeyRelease) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == s_DeviceButtonPress) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == s_DeviceButtonRelease) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == s_DeviceMotionNotify) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == s_DeviceFocusIn)
        DO_SWAP(SEventFocus, deviceFocus);
    else if (type == s_DeviceFocusOut)
        DO_SWAP(SEventFocus, deviceFocus);
    else if (type == s_ProximityIn) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == s_ProximityOut) {
        SKeyButtonPtrEvent(from, to);
        to->u.keyButtonPointer.pad1 = from->u.keyButtonPointer.pad1;
    }
    else if (type == s_DeviceStateNotify)
        DO_SWAP(SDeviceStateNotifyEvent, deviceStateNotify);
    else if (type == s_DeviceKeyStateNotify)
        DO_SWAP(SDeviceKeyStateNotifyEvent, deviceKeyStateNotify);
    else if (type == s_DeviceButtonStateNotify)
        DO_SWAP(SDeviceButtonStateNotifyEvent, deviceButtonStateNotify);
    else if (type == s_DeviceMappingNotify)
        DO_SWAP(SDeviceMappingNotifyEvent, deviceMappingNotify);
    else if (type == s_ChangeDeviceNotify)
        DO_SWAP(SChangeDeviceNotifyEvent, changeDeviceNotify);
    else if (type == s_DevicePresenceNotify)
        DO_SWAP(SDevicePresenceNotifyEvent, devicePresenceNotify);
    else if (type == s_DevicePropertyNotify)
        DO_SWAP(SDevicePropertyNotifyEvent, devicePropertyNotify);
    else {
        fprintf(stderr, "XInputExtension: Impossible event!\n");
        abort();
    }
}

/**********************************************************************
 *
 * IExtensionInit - initialize the input extension.
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 * This extension has several events and errors.
 *
 * XI is mandatory nowadays, so if we fail to init XI, we die.
 */

void
XInputExtensionInit(XephyrContext* context)
{
    ExtensionEntry *extEntry;

    XExtensionVersion thisversion = { XI_Present,
        SERVER_XI_MAJOR_VERSION,
        SERVER_XI_MINOR_VERSION,
    };

    if (!dixRegisterPrivateKey
        (&context->XIClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(XIClientRec), context))
        FatalError("Cannot request private for XI.\n", context);

    if (!XIBarrierInit(context))
        FatalError("Could not initialize barriers.\n", context);

    extEntry = AddExtension(INAME, IEVENTS, IERRORS, ProcIDispatch,
                            SProcIDispatch, IResetProc, StandardMinorOpcode, context);
    if (extEntry) {
        context->IReqCode = extEntry->base;
        context->IEventBase = extEntry->eventBase;
        context->XIVersion = thisversion;
        MakeDeviceTypeAtoms();
        context->RT_INPUTCLIENT = CreateNewResourceType((DeleteType) InputClientGone,
                                               "INPUTCLIENT", context);
        if (!context->RT_INPUTCLIENT)
            FatalError("Failed to add resource type for XI.\n", context);
        FixExtensionEvents(extEntry, context);
        ReplySwapVector[context->IReqCode] = (ReplySwapPtr) SReplyIDispatch;
        EventSwapVector[context->DeviceValuator] = SEventIDispatch;
        EventSwapVector[context->DeviceKeyPress] = SEventIDispatch;
        EventSwapVector[context->DeviceKeyRelease] = SEventIDispatch;
        EventSwapVector[context->DeviceButtonPress] = SEventIDispatch;
        EventSwapVector[context->DeviceButtonRelease] = SEventIDispatch;
        EventSwapVector[context->DeviceMotionNotify] = SEventIDispatch;
        EventSwapVector[context->DeviceFocusIn] = SEventIDispatch;
        EventSwapVector[context->DeviceFocusOut] = SEventIDispatch;
        EventSwapVector[context->ProximityIn] = SEventIDispatch;
        EventSwapVector[context->ProximityOut] = SEventIDispatch;
        EventSwapVector[context->DeviceStateNotify] = SEventIDispatch;
        EventSwapVector[context->DeviceKeyStateNotify] = SEventIDispatch;
        EventSwapVector[context->DeviceButtonStateNotify] = SEventIDispatch;
        EventSwapVector[context->DeviceMappingNotify] = SEventIDispatch;
        EventSwapVector[context->ChangeDeviceNotify] = SEventIDispatch;
        EventSwapVector[context->DevicePresenceNotify] = SEventIDispatch;

        GERegisterExtension(context->IReqCode, XI2EventSwap, context);

        memset(&xi_all_devices, 0, sizeof(xi_all_devices));
        memset(&xi_all_master_devices, 0, sizeof(xi_all_master_devices));
        xi_all_devices.id = XIAllDevices;
        xi_all_devices.name = strdup("XIAllDevices");
        xi_all_master_devices.id = XIAllMasterDevices;
        xi_all_master_devices.name = strdup("XIAllMasterDevices");

        context->inputInfo.all_devices = &xi_all_devices;
        context->inputInfo.all_master_devices = &xi_all_master_devices;

        XIResetProperties();
    }
    else {
        FatalError("IExtensionInit: AddExtensions failed\n", context);
    }
}
