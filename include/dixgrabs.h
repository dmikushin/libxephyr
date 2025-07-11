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

#ifndef DIXGRABS_H
#define DIXGRABS_H 1

struct _GrabParameters;
typedef struct _XephyrContext XephyrContext;

extern void PrintDeviceGrabInfo(DeviceIntPtr dev);
extern void UngrabAllDevices(Bool kill_client, XephyrContext* context);

extern GrabPtr AllocGrab(const GrabPtr src, XephyrContext* context);
extern void FreeGrab(GrabPtr grab, XephyrContext* context);
extern Bool CopyGrab(GrabPtr dst, const GrabPtr src, XephyrContext* context);

extern GrabPtr CreateGrab(int /* client */ ,
                          DeviceIntPtr /* device */ ,
                          DeviceIntPtr /* modDevice */ ,
                          WindowPtr /* window */ ,
                          enum InputLevel /* grabtype */ ,
                          GrabMask * /* mask */ ,
                          struct _GrabParameters * /* param */ ,
                          int /* type */ ,
                          KeyCode /* keybut */ ,
                          WindowPtr /* confineTo */ ,
                          CursorPtr /* cursor */ );

extern _X_EXPORT int DeletePassiveGrab(void *value,
                                       XID id,
                                       XephyrContext* context);

extern _X_EXPORT Bool GrabMatchesSecond(GrabPtr /* pFirstGrab */ ,
                                        GrabPtr /* pSecondGrab */ ,
                                        Bool /*ignoreDevice */ );

extern _X_EXPORT int AddPassiveGrabToList(ClientPtr /* client */ ,
                                          GrabPtr /* pGrab */ ,
                                          XephyrContext* /* context */ );

extern _X_EXPORT Bool DeletePassiveGrabFromList(GrabPtr /* pMinuendGrab */ );

extern Bool GrabIsPointerGrab(GrabPtr grab);
extern Bool GrabIsKeyboardGrab(GrabPtr grab);
extern Bool GrabIsGestureGrab(GrabPtr grab);
#endif                          /* DIXGRABS_H */
