#include "dix/context.h"

#ifndef _XSERV_GLOBAL_H_
#define _XSERV_GLOBAL_H_

#include "window.h"             /* for WindowPtr */
#include "extinit.h"
#ifdef DPMSExtension
/* sigh, too many drivers assume this */
#include <X11/extensions/dpmsconst.h>
#endif

/* Global X server variables that are visible to mi, dix, os, and ddx */



#ifdef PANORAMIX
extern _X_EXPORT Bool PanoramiXExtensionDisabledHack;
#endif

#ifdef XSELINUX
#define SELINUX_MODE_DEFAULT    0
#define SELINUX_MODE_DISABLED   1
#define SELINUX_MODE_PERMISSIVE 2
#define SELINUX_MODE_ENFORCING  3
extern _X_EXPORT int selinuxEnforcingState;
#endif

#endif                          /* !_XSERV_GLOBAL_H_ */
