/*
 * Copyright © 2010 NVIDIA Corporation
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
 */

#ifndef _SYNCSDK_H_
#define _SYNCSDK_H_

#include "misync.h"

extern _X_EXPORT int
 SyncVerifyFence(SyncFence ** ppFence, XID fid, ClientPtr client, Mask mode, XephyrContext* context);

extern _X_EXPORT SyncObject*
 SyncCreate(ClientPtr client, XID id, unsigned char type);

#define VERIFY_SYNC_FENCE(pFence, fid, client, mode)			\
    do {								\
	int rc;								\
	rc = SyncVerifyFence(&(pFence), (fid), (client), (mode), (client)->context);	\
	if (Success != rc) return rc;					\
    } while (0)

#define VERIFY_SYNC_FENCE_OR_NONE(pFence, fid, client, mode)		\
    do {								\
        pFence = 0;							\
        if (None != fid)						\
	    VERIFY_SYNC_FENCE((pFence), (fid), (client), (mode));	\
    } while (0)

#endif                          /* _SYNCSDK_H_ */
