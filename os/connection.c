#include <string.h>
#include "dix/context.h"
/***********************************************************

Copyright 1987, 1989, 1998  The Open Group

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

Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

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
/*****************************************************************
 *  Stuff to create connections --- OS dependent
 *
 *      EstablishNewConnections, CreateWellKnownSockets, ResetWellKnownSockets,
 *      CloseDownConnection,
 *	OnlyListToOneClient,
 *      ListenToAllClients,
 *
 *      (WaitForSomething is in its own file)
 *
 *      In this implementation, a client socket table is not kept.
 *      Instead, what would be the index into the table is just the
 *      file descriptor of the socket.  This won't work for if the
 *      socket ids aren't small nums (0 - 2^8)
 *
 *****************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef WIN32
#include <X11/Xwinsock.h>
#endif
#include <X11/X.h>
#include <X11/Xproto.h>
#define XSERV_t
#define TRANS_SERVER
#define TRANS_REOPEN
#include <X11/Xtrans/Xtrans.h>
#include <X11/Xtrans/Xtransint.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

#ifndef WIN32
#include <sys/socket.h>

#if defined(TCPCONN)
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef apollo
#ifndef NO_TCP_H
#include <netinet/tcp.h>
#endif
#else
#ifdef CSRG_BASED
#include <sys/param.h>
#endif
#include <netinet/tcp.h>
#endif
#include <arpa/inet.h>
#endif

#include <sys/uio.h>

#endif                          /* WIN32 */
#include "misc.h"               /* for typedef of pointer */
#include "osdep.h"
#include "opaque.h"
#include "dixstruct.h"
#include "xace.h"

#define Pid_t pid_t

#ifdef HAVE_GETPEERUCRED
#include <ucred.h>
#include <zone.h>
#else
#define zoneid_t int
#endif

#ifdef HAVE_SYSTEMD_DAEMON
#include <systemd/sd-daemon.h>
#endif

#include "probes.h"

/* server_poll now in XephyrContext */

/* REMOVED: Bool NoListenAll; - moved to XephyrContext */

static Bool RunFromSmartParent; /* send SIGUSR1 to parent process */
/* REMOVED: Bool RunFromSigStopParent; - moved to XephyrContext */
static char dynamic_display[7]; /* display name */
/* REMOVED: Bool PartialNetwork; - moved to XephyrContext */
/* static Pid_t context->ParentProcess; */

// // int GrabInProgress = 0;

static void
EstablishNewConnections(int curconn, int ready, void *data);

static void
set_poll_client(ClientPtr client);

static void
set_poll_clients(XephyrContext* context);

/* static XtransConnInfo *context->ListenTransConns = NULL; */
/* static int *context->ListenTransFds = NULL; */
/* static int context->ListenTransCount; */

struct ConnMaxData {
    XtransConnInfo trans_conn;
    XephyrContext *context;
};

static void ErrorConnMax(XtransConnInfo trans_conn, XephyrContext *context);

static XtransConnInfo
lookup_trans_conn(int fd, XephyrContext *context)
{
    if (context->ListenTransFds) {
        int i;

        for (i = 0; i < context->ListenTransCount; i++)
            if (context->ListenTransFds[i] == fd)
                return context->ListenTransConns[i];
    }

    return NULL;
}

/*
 * If SIGUSR1 was set to SIG_IGN when the server started, assume that either
 *
 *  a- The parent process is ignoring SIGUSR1
 *
 * or
 *
 *  b- The parent process is expecting a SIGUSR1
 *     when the server is ready to accept connections
 *
 * In the first case, the signal will be harmless, in the second case,
 * the signal will be quite useful.
 */
static void
InitParentProcess(XephyrContext* context)
{
#if !defined(WIN32)
    OsSigHandlerPtr handler;

    handler = OsSignal(SIGUSR1, SIG_IGN);
    if (handler == SIG_IGN)
        RunFromSmartParent = TRUE;
    OsSignal(SIGUSR1, handler);
    context->ParentProcess = getppid();
#endif
}

void
NotifyParentProcess(XephyrContext* context)
{
#if !defined(WIN32)
    if (context->displayfd >= 0) {
        if (write(context->displayfd, context->display, strlen(context->display)) != strlen(context->display))
            FatalError("Cannot write context->display number to fd %d\n", context, context->displayfd);
        if (write(context->displayfd, "\n", 1) != 1)
            FatalError("Cannot write context->display number to fd %d\n", context, context->displayfd);
        close(context->displayfd);
        context->displayfd = -1;
    }
    if (RunFromSmartParent) {
        if (context->ParentProcess > 1) {
            kill(context->ParentProcess, SIGUSR1);
        }
    }
    if (context->RunFromSigStopParent)
        raise(SIGSTOP);
#ifdef HAVE_SYSTEMD_DAEMON
    /* If we have been started as a systemd service, tell systemd that
       we are ready. Otherwise sd_notify() won't do anything. */
    sd_notify(0, "READY=1");
#endif
#endif
}

static Bool
TryCreateSocket(int num, int *partial, XephyrContext* context)
{
    char port[20];

    snprintf(port, sizeof(port), "%d", num);

    return (_XSERVTransMakeAllCOTSServerListeners(port, partial,
                                                  &context->ListenTransCount,
                                                  &context->ListenTransConns) >= 0);
}

/*****************
 * CreateWellKnownSockets
 *    At initialization, create the sockets to listen on for new context->clients.
 *****************/

void
CreateWellKnownSockets(XephyrContext* context)
{
    int i;
    int partial;

    /* context->display is initialized to "0" by main(). It is then set to the context->display
     * number if specified on the command line. */

    if (context->NoListenAll) {
        context->ListenTransCount = 0;
    }
    else if ((context->displayfd < 0) || context->explicit_display) {
        if (TryCreateSocket(atoi(context->display), &partial, context) &&
            context->ListenTransCount >= 1)
            if (!context->PartialNetwork && partial)
                FatalError("Failed to establish all listening sockets", context);
    }
    else { /* -context->displayfd and no explicit context->display number */
        Bool found = 0;
        for (i = 0; i < 65536 - X_TCP_PORT; i++) {
            if (TryCreateSocket(i, &partial, context) && !partial) {
                found = 1;
                break;
            }
            else
                CloseWellKnownConnections(context);
        }
        if (!found)
            FatalError("Failed to find a socket to listen on", context);
        snprintf(dynamic_display, sizeof(dynamic_display), "%d", i);
        context->display = dynamic_display;
        LogSetDisplay(context);
    }

    context->ListenTransFds = xallocarray(context->ListenTransCount, sizeof (int));
    if (context->ListenTransFds == NULL)
        FatalError("Failed to create listening socket array", context);

    for (i = 0; i < context->ListenTransCount; i++) {
        int fd = _XSERVTransGetConnectionNumber(context->ListenTransConns[i]);

        context->ListenTransFds[i] = fd;
        SetNotifyFd(fd, EstablishNewConnections, X_NOTIFY_READ, context, context);

        if (!_XSERVTransIsLocal(context->ListenTransConns[i]))
            DefineSelf (fd, context);
    }

    if (context->ListenTransCount == 0 && !context->NoListenAll)
        FatalError("Cannot establish any listening sockets - Make sure an X server isn't already running", context);

#if !defined(WIN32)
    OsSignal(SIGPIPE, SIG_IGN);
    OsSignal(SIGHUP, AutoResetServer);
#endif
    OsSignal(SIGINT, GiveUp);
    OsSignal(SIGTERM, GiveUp);
    ResetHosts(context->display, context);

    InitParentProcess(context);

#ifdef XDMCP
    XdmcpInit(context);
#endif
}

void
ResetWellKnownSockets(XephyrContext* context)
{
    int i;

    ResetOsBuffers();

    for (i = 0; i < context->ListenTransCount; i++) {
        int status = _XSERVTransResetListener(context->ListenTransConns[i]);

        if (status != TRANS_RESET_NOOP) {
            if (status == TRANS_RESET_FAILURE) {
                /*
                 * context->ListenTransConns[i] freed by xtrans.
                 * Remove it from out list.
                 */

                RemoveNotifyFd(context->ListenTransFds[i], context);
                context->ListenTransFds[i] = context->ListenTransFds[context->ListenTransCount - 1];
                context->ListenTransConns[i] = context->ListenTransConns[context->ListenTransCount - 1];
                context->ListenTransCount -= 1;
                i -= 1;
            }
            else if (status == TRANS_RESET_NEW_FD) {
                /*
                 * A new file descriptor was allocated (the old one was closed)
                 */

                int newfd = _XSERVTransGetConnectionNumber(context->ListenTransConns[i]);

                context->ListenTransFds[i] = newfd;
            }
        }
    }
    for (i = 0; i < context->ListenTransCount; i++)
        SetNotifyFd(context->ListenTransFds[i], EstablishNewConnections, X_NOTIFY_READ,
                    context, context);

    ResetAuthorization();
    ResetHosts(context->display, context);
    /*
     * restart XDMCP
     */
#ifdef XDMCP
    XdmcpReset(context);
#endif
}

void
CloseWellKnownConnections(XephyrContext* context)
{
    int i;

    for (i = 0; i < context->ListenTransCount; i++) {
        if (context->ListenTransConns[i] != NULL) {
            _XSERVTransClose(context->ListenTransConns[i]);
            context->ListenTransConns[i] = NULL;
            if (context->ListenTransFds != NULL)
                RemoveNotifyFd(context->ListenTransFds[i], context);
        }
    }
    context->ListenTransCount = 0;
}

static void
AuthAudit(ClientPtr client, Bool letin,
          struct sockaddr *saddr, int len,
          unsigned int proto_n, char *auth_proto, int auth_id)
{
    char addr[128];
    char client_uid_string[64];
    LocalClientCredRec *lcc;

#ifdef XSERVER_DTRACE
    pid_t client_pid = -1;
    zoneid_t client_zid = -1;
#endif

    if (!len)
        strlcpy(addr, "local host", sizeof(addr));
    else
        switch (saddr->sa_family) {
        case AF_UNSPEC:
#if defined(UNIXCONN) || defined(LOCALCONN)
        case AF_UNIX:
#endif
            strlcpy(addr, "local host", sizeof(addr));
            break;
#if defined(TCPCONN)
        case AF_INET:
            snprintf(addr, sizeof(addr), "IP %s",
                     inet_ntoa(((struct sockaddr_in *) saddr)->sin_addr));
            break;
#if defined(IPv6) && defined(AF_INET6)
        case AF_INET6:{
            char ipaddr[INET6_ADDRSTRLEN];

            inet_ntop(AF_INET6, &((struct sockaddr_in6 *) saddr)->sin6_addr,
                      ipaddr, sizeof(ipaddr));
            snprintf(addr, sizeof(addr), "IP %s", ipaddr);
        }
            break;
#endif
#endif
        default:
            strlcpy(addr, "unknown address", sizeof(addr));
        }

    if (GetLocalClientCreds(client, &lcc) != -1) {
        int slen;               /* length written to client_uid_string */

        strcpy(client_uid_string, " ( ");
        slen = 3;

        if (lcc->fieldsSet & LCC_UID_SET) {
            snprintf(client_uid_string + slen,
                     sizeof(client_uid_string) - slen,
                     "uid=%ld ", (long) lcc->euid);
            slen = strlen(client_uid_string);
        }

        if (lcc->fieldsSet & LCC_GID_SET) {
            snprintf(client_uid_string + slen,
                     sizeof(client_uid_string) - slen,
                     "gid=%ld ", (long) lcc->egid);
            slen = strlen(client_uid_string);
        }

        if (lcc->fieldsSet & LCC_PID_SET) {
#ifdef XSERVER_DTRACE
            client_pid = lcc->pid;
#endif
            snprintf(client_uid_string + slen,
                     sizeof(client_uid_string) - slen,
                     "pid=%ld ", (long) lcc->pid);
            slen = strlen(client_uid_string);
        }

        if (lcc->fieldsSet & LCC_ZID_SET) {
#ifdef XSERVER_DTRACE
            client_zid = lcc->zoneid;
#endif
            snprintf(client_uid_string + slen,
                     sizeof(client_uid_string) - slen,
                     "zoneid=%ld ", (long) lcc->zoneid);
            slen = strlen(client_uid_string);
        }

        snprintf(client_uid_string + slen, sizeof(client_uid_string) - slen,
                 ")");
        FreeLocalClientCreds(lcc);
    }
    else {
        client_uid_string[0] = '\0';
    }

#ifdef XSERVER_DTRACE
    XSERVER_CLIENT_AUTH(client->index, addr, client_pid, client_zid);
#endif
    if (auditTrailLevel > 1) {
        if (proto_n)
            AuditF("client %d %s from %s%s\n  Auth name: %.*s ID: %d\n",
                   client->index, letin ? "connected" : "rejected", addr,
                   client_uid_string, (int) proto_n, auth_proto, auth_id);
        else
            AuditF("client %d %s from %s%s\n",
                   client->index, letin ? "connected" : "rejected", addr,
                   client_uid_string);

    }
}

XID
AuthorizationIDOfClient(ClientPtr client)
{
    if (client->osPrivate)
        return ((OsCommPtr) client->osPrivate)->auth_id;
    else
        return None;
}

/*****************************************************************
 * ClientAuthorized
 *
 *    Sent by the client at connection setup:
 *                typedef struct _xConnClientPrefix {
 *                   CARD8	byteOrder;
 *                   BYTE	pad;
 *                   CARD16	majorVersion, minorVersion;
 *                   CARD16	nbytesAuthProto;
 *                   CARD16	nbytesAuthString;
 *                 } xConnClientPrefix;
 *
 *     	It is hoped that eventually one protocol will be agreed upon.  In the
 *        mean time, a server that implements a different protocol than the
 *        client expects, or a server that only implements the host-based
 *        mechanism, will simply ignore this information.
 *
 *****************************************************************/

const char *
ClientAuthorized(ClientPtr client,
                 unsigned int proto_n, char *auth_proto,
                 unsigned int string_n, char *auth_string)
{
    OsCommPtr priv;
    Xtransaddr *from = NULL;
    int family;
    int fromlen;
    XID auth_id;
    const char *reason = NULL;
    XtransConnInfo trans_conn;

    priv = (OsCommPtr) client->osPrivate;
    trans_conn = priv->trans_conn;

    /* Allow any client to connect without authorization on a launchd socket,
       because it is securely created -- this prevents a race condition on launch */
    if (trans_conn->flags & TRANS_NOXAUTH) {
        auth_id = (XID) 0L;
    }
    else {
        auth_id =
            CheckAuthorization(proto_n, auth_proto, string_n, auth_string,
                               client, &reason);
    }

    if (auth_id == (XID) ~0L) {
        if (_XSERVTransGetPeerAddr(trans_conn, &family, &fromlen, &from) != -1) {
            if (InvalidHost((struct sockaddr *) from, fromlen, client))
                AuthAudit(client, FALSE, (struct sockaddr *) from,
                          fromlen, proto_n, auth_proto, auth_id);
            else {
                auth_id = (XID) 0;
#ifdef XSERVER_DTRACE
                if ((auditTrailLevel > 1) || XSERVER_CLIENT_AUTH_ENABLED())
#else
                if (auditTrailLevel > 1)
#endif
                    AuthAudit(client, TRUE,
                              (struct sockaddr *) from, fromlen,
                              proto_n, auth_proto, auth_id);
            }

            free(from);
        }

        if (auth_id == (XID) ~0L) {
            if (reason)
                return reason;
            else
                return "Client is not authorized to connect to Server";
        }
    }
#ifdef XSERVER_DTRACE
    else if ((auditTrailLevel > 1) || XSERVER_CLIENT_AUTH_ENABLED())
#else
    else if (auditTrailLevel > 1)
#endif
    {
        if (_XSERVTransGetPeerAddr(trans_conn, &family, &fromlen, &from) != -1) {
            AuthAudit(client, TRUE, (struct sockaddr *) from, fromlen,
                      proto_n, auth_proto, auth_id);

            free(from);
        }
    }
    priv->auth_id = auth_id;
    priv->conn_time = 0;

#ifdef XDMCP
    /* indicate to Xdmcp protocol that we've opened new client */
    XdmcpOpenDisplay(priv->fd);
#endif                          /* XDMCP */

    XaceHook(XACE_AUTH_AVAIL, client, auth_id);

    /* At this point, if the client is authorized to change the access control
     * list, we should getpeername() information, and add the client to
     * the selfhosts list.  It's not really the host machine, but the
     * true purpose of the selfhosts list is to see who may change the
     * access control list.
     */
    return ((char *) NULL);
}

static void
ClientReady(int fd, int xevents, void *data)
{
    ClientPtr client = data;

    if (xevents & X_NOTIFY_ERROR) {
        CloseDownClient(client);
        return;
    }
    if (xevents & X_NOTIFY_READ)
        mark_client_ready(client);
    if (xevents & X_NOTIFY_WRITE) {
        ospoll_mute(client->context->server_poll, fd, X_NOTIFY_WRITE);
        client->context->NewOutputPending = TRUE;
    }
}

static ClientPtr
AllocNewConnection(XtransConnInfo trans_conn, int fd, CARD32 conn_time, XephyrContext *context)
{
    OsCommPtr oc;
    ClientPtr client;

    oc = malloc(sizeof(OsCommRec));
    if (!oc)
        return NullClient;
    oc->trans_conn = trans_conn;
    oc->fd = fd;
    oc->input = (ConnectionInputPtr) NULL;
    oc->output = (ConnectionOutputPtr) NULL;
    oc->auth_id = None;
    oc->conn_time = conn_time;
    oc->flags = 0;
    if (!(client = NextAvailableClient((void *) oc, context))) {
        free(oc);
        return NullClient;
    }
    client->local = ComputeLocalClient(client);
    ospoll_add(context->server_poll, fd,
               ospoll_trigger_edge,
               ClientReady,
               client);
    set_poll_client(client);

#ifdef DEBUG
    ErrorF("AllocNewConnection: client index = %d, socket fd = %d\n", context,
           client->index, fd);
#endif
#ifdef XSERVER_DTRACE
    XSERVER_CLIENT_CONNECT(client->index, fd);
#endif

    return client;
}

/*****************
 * EstablishNewConnections
 *    If anyone is waiting on listened sockets, accept them. Drop pending
 *    connections if they've stuck around for more than one minute.
 *****************/
#define TimeOutValue 60 * MILLI_PER_SECOND
static void
EstablishNewConnections(int curconn, int ready, void *data)
{
    int newconn;       /* fd of new client */
    CARD32 connect_time;
    int i;
    ClientPtr client;
    OsCommPtr oc;
    XtransConnInfo trans_conn, new_trans_conn;
    int status;
    XephyrContext *context = (XephyrContext *)data;

    connect_time = GetTimeInMillis();
    /* kill off stragglers */
    for (i = 1; i < context->currentMaxClients; i++) {
        if ((client = context->clients[i])) {
            oc = (OsCommPtr) (client->osPrivate);
            if ((oc && (oc->conn_time != 0) &&
                 (connect_time - oc->conn_time) >= TimeOutValue) ||
                (client->noClientException != Success && !client->clientGone))
                CloseDownClient(client);
        }
    }

    if ((trans_conn = lookup_trans_conn(curconn, context)) == NULL)
        return;

    if ((new_trans_conn = _XSERVTransAccept(trans_conn, &status)) == NULL)
        return;

    newconn = _XSERVTransGetConnectionNumber(new_trans_conn);

    _XSERVTransSetOption(new_trans_conn, TRANS_NONBLOCKING, 1);

    if (trans_conn->flags & TRANS_NOXAUTH)
        new_trans_conn->flags = new_trans_conn->flags | TRANS_NOXAUTH;

    if (!AllocNewConnection(new_trans_conn, newconn, connect_time, context)) {
        ErrorConnMax(new_trans_conn, context);
    }
    return;
}

#define NOROOM "Maximum number of context->clients reached"

/************
 *   ErrorConnMax
 *     Fail a connection due to lack of client or file descriptor space
 ************/

static void
ConnMaxNotify(int fd, int events, void *data)
{
    struct ConnMaxData *conn_data = (struct ConnMaxData *)data;
    XtransConnInfo trans_conn = conn_data->trans_conn;
    XephyrContext *context = conn_data->context;
    char order = 0;

    /* try to read the byte-order of the connection */
    (void) _XSERVTransRead(trans_conn, &order, 1);
    if (order == 'l' || order == 'B' || order == 'r' || order == 'R') {
        xConnSetupPrefix csp;
        char pad[3] = { 0, 0, 0 };
        int whichbyte = 1;
        struct iovec iov[3];

        csp.success = xFalse;
        csp.lengthReason = sizeof(NOROOM) - 1;
        csp.length = (sizeof(NOROOM) + 2) >> 2;
        csp.majorVersion = X_PROTOCOL;
        csp.minorVersion = X_PROTOCOL_REVISION;
        if (((*(char *) &whichbyte) && (order == 'B' || order == 'R')) ||
            (!(*(char *) &whichbyte) && (order == 'l' || order == 'r'))) {
            swaps(&csp.majorVersion);
            swaps(&csp.minorVersion);
            swaps(&csp.length);
        }
        iov[0].iov_len = sz_xConnSetupPrefix;
        iov[0].iov_base = (char *) &csp;
        iov[1].iov_len = csp.lengthReason;
        iov[1].iov_base = (void *) NOROOM;
        iov[2].iov_len = (4 - (csp.lengthReason & 3)) & 3;
        iov[2].iov_base = pad;
        (void) _XSERVTransWritev(trans_conn, iov, 3);
    }
    RemoveNotifyFd(trans_conn->fd, context);
    _XSERVTransClose(trans_conn);
    free(conn_data);
}

static void
ErrorConnMax(XtransConnInfo trans_conn, XephyrContext *context)
{
    struct ConnMaxData *conn_data = malloc(sizeof(struct ConnMaxData));
    if (!conn_data) {
        _XSERVTransClose(trans_conn);
        return;
    }
    conn_data->trans_conn = trans_conn;
    conn_data->context = context;
    
    if (!SetNotifyFd(trans_conn->fd, ConnMaxNotify, X_NOTIFY_READ, conn_data, context)) {
        free(conn_data);
        _XSERVTransClose(trans_conn);
    }
}

/************
 *   CloseDownFileDescriptor:
 *     Remove this file descriptor
 ************/

void
CloseDownFileDescriptor(OsCommPtr oc, XephyrContext* context)
{
    if (oc->trans_conn) {
        int connection = oc->fd;
#ifdef XDMCP
        XdmcpCloseDisplay(connection);
#endif
        ospoll_remove(context->server_poll, connection);
        _XSERVTransDisconnect(oc->trans_conn);
        _XSERVTransClose(oc->trans_conn);
        oc->trans_conn = NULL;
        oc->fd = -1;
    }
}

/*****************
 * CloseDownConnection
 *    Delete client from AllClients and free resources
 *****************/

void
CloseDownConnection(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    if (client->context && client->context->FlushCallback)
        CallCallbacks(&client->context->FlushCallback, client);

    if (oc->output)
	FlushClient(client, oc, (char *) NULL, 0);
    CloseDownFileDescriptor(oc, client->context);
    FreeOsBuffers(oc, client->context);
    free(client->osPrivate);
    client->osPrivate = (void *) NULL;
    if (auditTrailLevel > 1)
        AuditF("client %d disconnected\n", client->index);
}

struct notify_fd {
    int mask;
    NotifyFdProcPtr notify;
    void *data;
};

/*****************
 * HandleNotifyFd
 *    A poll callback to be called when the registered
 *    file descriptor is ready.
 *****************/

static void
HandleNotifyFd(int fd, int xevents, void *data)
{
    struct notify_fd *n = data;
    n->notify(fd, xevents, n->data);
}

/*****************
 * SetNotifyFd
 *    Registers a callback to be invoked when the specified
 *    file descriptor becomes readable.
 *****************/

Bool
SetNotifyFd(int fd, NotifyFdProcPtr notify, int mask, void *data, XephyrContext* context)
{
    struct notify_fd *n;

    n = ospoll_data(context->server_poll, fd);
    if (!n) {
        if (mask == 0)
            return TRUE;

        n = calloc(1, sizeof (struct notify_fd));
        if (!n)
            return FALSE;
        ospoll_add(context->server_poll, fd,
                   ospoll_trigger_level,
                   HandleNotifyFd,
                   n);
    }

    if (mask == 0) {
        ospoll_remove(context->server_poll, fd);
        free(n);
    } else {
        int listen = mask & ~n->mask;
        int mute = n->mask & ~mask;

        if (listen)
            ospoll_listen(context->server_poll, fd, listen);
        if (mute)
            ospoll_mute(context->server_poll, fd, mute);
        n->mask = mask;
        n->data = data;
        n->notify = notify;
    }

    return TRUE;
}

/*****************
 * OnlyListenToOneClient:
 *    Only accept requests from  one client.  Continue to handle new
 *    connections, but don't take any protocol requests from the new
 *    ones.  Note that if context->GrabInProgress is set, EstablishNewConnections
 *    needs to put new context->clients into SavedAllSockets and SavedAllClients.
 *    Note also that there is no timeout for this in the protocol.
 *    This routine is "undone" by ListenToAllClients()
 *****************/

int
OnlyListenToOneClient(ClientPtr client)
{
    int rc;

    rc = XaceHook(XACE_SERVER_ACCESS, client, DixGrabAccess);
    if (rc != Success)
        return rc;

    if (!client->context->GrabInProgress) {
        client->context->GrabInProgress = client->index;
        set_poll_clients(client->context);
    }

    return rc;
}

/****************
 * ListenToAllClients:
 *    Undoes OnlyListentToOneClient()
 ****************/

void
ListenToAllClients(XephyrContext* context)
{
    if (context->GrabInProgress) {
        context->GrabInProgress = 0;
        set_poll_clients(context);
    }
}

/****************
 * IgnoreClient
 *    Removes one client from input masks.
 *    Must have corresponding call to AttendClient.
 ****************/

void
IgnoreClient(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    client->ignoreCount++;
    if (client->ignoreCount > 1)
        return;

    client->context->isItTimeToYield = TRUE;
    mark_client_not_ready(client);

    oc->flags |= OS_COMM_IGNORED;
    set_poll_client(client);
}

/****************
 * AttendClient
 *    Adds one client back into the input masks.
 ****************/

void
AttendClient(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    if (client->clientGone) {
        /*
         * client is gone, so any pending requests will be dropped and its
         * ignore count doesn't matter.
         */
        return;
    }

    client->ignoreCount--;
    if (client->ignoreCount)
        return;

    oc->flags &= ~OS_COMM_IGNORED;
    set_poll_client(client);
    if (listen_to_client(client))
        mark_client_ready(client);
    else {
        /* grab active, mark ready when grab goes away */
        mark_client_saved_ready(client);
    }
}

/* make client impervious to grabs; assume only executing client calls this */

void
MakeClientGrabImpervious(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    oc->flags |= OS_COMM_GRAB_IMPERVIOUS;
    set_poll_client(client);

    if (client->context->ServerGrabCallback) {
        ServerGrabInfoRec grabinfo;

        grabinfo.client = client;
        grabinfo.grabstate = CLIENT_IMPERVIOUS;
        CallCallbacks(&client->context->ServerGrabCallback, &grabinfo);
    }
}

/* make client pervious to grabs; assume only executing client calls this */

void
MakeClientGrabPervious(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    oc->flags &= ~OS_COMM_GRAB_IMPERVIOUS;
    set_poll_client(client);
    client->context->isItTimeToYield = TRUE;

    if (client->context->ServerGrabCallback) {
        ServerGrabInfoRec grabinfo;

        grabinfo.client = client;
        grabinfo.grabstate = CLIENT_PERVIOUS;
        CallCallbacks(&client->context->ServerGrabCallback, &grabinfo);
    }
}

/* Add a fd (from launchd or similar) to our listeners */
void
ListenOnOpenFD(int fd, int noxauth, XephyrContext* context)
{
    char port[PATH_MAX];
    XtransConnInfo ciptr;
    const char *display_env = getenv("DISPLAY");

    /* First check if display_env matches a <absolute path to unix socket>[.<screen number>] scheme (eg: launchd) */
    if (display_env && display_env[0] == '/') {
        struct stat sbuf;

        strlcpy(port, display_env, sizeof(port));

        /* If the path exists, we don't have do do anything else.
         * If it doesn't, we need to check for a .<screen number> to strip off and recheck.
         */
        if (0 != stat(port, &sbuf)) {
            char *dot = strrchr(port, '.');
            if (dot) {
                *dot = '\0';

                if (0 != stat(port, &sbuf)) {
                    display_env = NULL;
                }
            } else {
                display_env = NULL;
            }
        }
    }

    if (!display_env) {
        /* Just some default so things don't break and die. */
        snprintf(port, sizeof(port), ":%d", atoi(context->display));
    }

    /* Make our XtransConnInfo
     * TRANS_SOCKET_LOCAL_INDEX = 5 from Xtrans.c
     */
    ciptr = _XSERVTransReopenCOTSServer(5, fd, port);
    if (ciptr == NULL) {
        ErrorF("Got NULL while trying to Reopen listen port.\n", context);
        return;
    }

    if (noxauth)
        ciptr->flags = ciptr->flags | TRANS_NOXAUTH;

    /* Allocate space to store it */
    context->ListenTransFds =
        XNFreallocarray(context->ListenTransFds, context->ListenTransCount + 1, sizeof(int), context);
    context->ListenTransConns =
        XNFreallocarray(context->ListenTransConns, context->ListenTransCount + 1,
                        sizeof(XtransConnInfo), context);

    /* Store it */
    context->ListenTransConns[context->ListenTransCount] = ciptr;
    context->ListenTransFds[context->ListenTransCount] = fd;

    SetNotifyFd(fd, EstablishNewConnections, X_NOTIFY_READ, context, context);

    /* Increment the count */
    context->ListenTransCount++;
}

/* based on TRANS(SocketUNIXAccept) (XtransConnInfo ciptr, int *status) */
Bool
AddClientOnOpenFD(int fd, XephyrContext *context)
{
    XtransConnInfo ciptr;
    CARD32 connect_time;
    char port[20];

    snprintf(port, sizeof(port), ":%d", atoi(context->display));
    ciptr = _XSERVTransReopenCOTSServer(5, fd, port);
    if (ciptr == NULL)
        return FALSE;

    _XSERVTransSetOption(ciptr, TRANS_NONBLOCKING, 1);
    ciptr->flags |= TRANS_NOXAUTH;

    connect_time = GetTimeInMillis();

    if (!AllocNewConnection(ciptr, fd, connect_time, context)) {
        ErrorConnMax(ciptr, context);
        return FALSE;
    }

    return TRUE;
}

Bool
listen_to_client(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    if (oc->flags & OS_COMM_IGNORED)
        return FALSE;

    if (!client->context->GrabInProgress)
        return TRUE;

    if (client->index == client->context->GrabInProgress)
        return TRUE;

    if (oc->flags & OS_COMM_GRAB_IMPERVIOUS)
        return TRUE;

    return FALSE;
}

static void
set_poll_client(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    if (oc->trans_conn) {
        if (listen_to_client(client))
            ospoll_listen(client->context->server_poll, oc->trans_conn->fd, X_NOTIFY_READ);
        else
            ospoll_mute(client->context->server_poll, oc->trans_conn->fd, X_NOTIFY_READ);
    }
}

static void
set_poll_clients(XephyrContext* context)
{
    int i;

    for (i = 1; i < context->currentMaxClients; i++) {
        ClientPtr client = context->clients[i];
        if (client && !client->clientGone)
            set_poll_client(client);
    }
}
