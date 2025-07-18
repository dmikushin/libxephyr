/* Prototypes for functions that the DDX must provide */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _DPMSPROC_H_
#define _DPMSPROC_H_

#include "dixstruct.h"

extern int DPMSSet(ClientPtr client, int level);
extern Bool DPMSSupported(XephyrContext* context);

#endif
