#include "dix/context.h"
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xfuncproto.h>
#include <stdarg.h>

/* Wrapper for external Xtrans library that expects old VErrorF signature */
static void XtransVErrorF(const char *f, va_list args)
{
    /* Since we're in external library context, we don't have access to context.
     * This is a compatibility wrapper for Xtrans transport.c */
    vfprintf(stderr, f, args);
}

/* Override VErrorF macro for Xtrans */
#define VErrorF XtransVErrorF

/* ErrorF is used by xtrans */
#ifndef HAVE_DIX_CONFIG_H
extern _X_EXPORT void
ErrorF(const char *f, ...)
_X_ATTRIBUTE_PRINTF(1, 2);
#endif

#define TRANS_REOPEN
#define TRANS_SERVER
#define XSERV_t
#include <X11/Xtrans/transport.c>
