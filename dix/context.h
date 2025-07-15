
#ifndef DIX_CONTEXT_H
#define DIX_CONTEXT_H

#include "dix-config.h"
#include <X11/X.h>
#include <X11/Xmd.h>
#include "dixstruct.h"
#include "scrnintstr.h"
#include "input.h"
#include "inputstr.h"
#include "client.h"
#include <X11/fonts/font.h>
#include "cursor.h"
#include "misc.h"
#include "regionstr.h"
#include "selection.h"
#include "os.h"
#include "../os/osdep.h"
#include <sys/select.h>
#include "opaque.h"
#include <X11/Xproto.h>
#include "../os/ospoll.h"
#include "servermd.h"
#include "privates.h"
#include "resource.h"
#include "list.h"
#include <X11/extensions/render.h>
#include <X11/extensions/xfixeswire.h>
#include <X11/extensions/XI.h>
#include <xcb/xcb.h>
#include <xcb/render.h>

/* Forward declarations for Phase 3 types */
struct _XtransConnInfo;
typedef struct _XtransConnInfo *XtransConnInfo;

/* EphyrHostXVars structure from hw/kdrive/ephyr/hostx.c */
struct EphyrHostXVars {
    char *server_dpy_name;
    xcb_connection_t *conn;
    int screen;
    xcb_visualtype_t *visual;
    Window winroot;
    xcb_gcontext_t  gc;
    xcb_render_pictformat_t argb_format;
    xcb_cursor_t empty_cursor;
    xcb_generic_event_t *saved_event;
    int depth;
    Bool use_sw_cursor;
    Bool use_fullscreen;
    Bool have_shm;
    Bool have_shm_fd_passing;

    int n_screens;
    void **screens;  /* KdScreenInfo ** */

    long damage_debug_msec;
    Bool size_set_from_configure;
};
typedef struct EphyrHostXVars EphyrHostXVars;

#ifdef CONFIG_UDEV
#include <libudev.h>
#endif

#ifndef RenderNumberRequests
#define RenderNumberRequests 37
#endif
#ifndef XFixesNumberRequests  
#define XFixesNumberRequests 35
#endif

typedef struct _XephyrContext {
    ScreenInfo screenInfo;
    InputInfo inputInfo;
    KeybdCtrl defaultKeyboardControl;
    PtrCtrl defaultPointerControl;
    ClientPtr clients[MAXCLIENTS];
    ClientPtr serverClient;
    int currentMaxClients;
    long maxBigRequestSize;
    unsigned long globalSerialNumber;
    unsigned long serverGeneration;
    CARD32 ScreenSaverTime;
    CARD32 ScreenSaverInterval;
    int ScreenSaverBlanking;
    int ScreenSaverAllowExposures;
    CARD32 defaultScreenSaverTime;
    CARD32 defaultScreenSaverInterval;
    int defaultScreenSaverBlanking;
    int defaultScreenSaverAllowExposures;
    Bool screenSaverSuspended;
    const char * defaultFontPath;
    FontPtr defaultFont;
    CursorPtr rootCursor;
    Bool party_like_its_1989;
    Bool whiteRoot;
    TimeStamp currentTime;
    int defaultColorVisualClass;
    int monitorResolution;
    const char * display;
    int displayfd;
    Bool explicit_display;
    char * ConnectionInfo;
    
    // OS/backing store variables
    Bool enableBackingStore;
    Bool disableBackingStore;
    
    // Resource limit variables  
    int limitDataSpace;
    int limitNoFile;
    int limitStackSpace;
    
    // Background/window variables
    Bool bgNoneRoot;
    
    // Process control variables
    Bool RunFromSigStopParent;
    
    // Access control variables
    Bool defeatAccessControl;
    
    // Network variables
    Bool PartialNetwork;
    
    // Signal handling variables
    Bool inSignalContext;
    
    // Extension control variables
    Bool noTestExtensions;
    int GrabInProgress;
    
    // Seat configuration
    char *SeatId;
    
#ifdef CONFIG_UDEV
    struct udev_monitor *udev_monitor;
#endif
    
    // GLX configuration
    Bool enableIndirectGLX;
    
    // Global variables from nm output
    HWEventQueuePtr checkForInput[2];
    CallbackListPtr ClientStateCallback;
    int connBlockScreenStart;
    xConnSetupPrefix connSetupPrefix;
    volatile char dispatchException;
    OsTimerPtr dispatchExceptionTimer;
    Bool isItTimeToYield;
    struct xorg_list output_pending_clients;
    PaddingInfo PixmapWidthPaddingInfo[33];
    CallbackListPtr ServerGrabCallback;
    Bool SmartScheduleLatencyLimited;
    long SmartScheduleTime;
    int terminateDelay;
    void* fontPatternCache;
    CallbackListPtr RootWindowFinalizeCallback;
    void* lastGLContext;
    WorkQueuePtr workQueue;
    CallbackListPtr DeviceEventCallback;
    Mask DontPropagateMasks[MAXDEVICES];
    CallbackListPtr EventCallback;
    Mask event_filters[MAXDEVICES][MAXEVENTS];
    struct {
        Bool playingEvents;
        TimeStamp time;
        DeviceIntPtr replayDev;
        WindowPtr replayWin;
        struct xorg_list pending;
    } syncEvents;
    InternalEvent *InputEventList;
    CallbackListPtr PropertyStateCallback;
    RegDataRec RegionBrokenData;
    BoxRec RegionEmptyBox;
    RegDataRec RegionEmptyData;
    RESTYPE RegionResType;
    RegionRec RegionBrokenRegion;  /* Special broken region for error handling */
    RESTYPE lastResourceType;
    CallbackListPtr ResourceStateCallback;
    RESTYPE TypeMask;
    Selection *CurrentSelections;
    CallbackListPtr SelectionCallback;
    Bool NewOutputPending;
    Bool NoListenAll;
    struct ospoll* server_poll;
    Bool InputThreadEnable;
    CallbackListPtr FlushCallback;
    CallbackListPtr ReplyCallback;
    Bool CoreDump;
    Bool noCompositeExtension;
    Bool noDamageExtension;
    Bool noDbeExtension;
    Bool noDRI2Extension;
    Bool noGEExtension;
    Bool noGlxExtension;
    Bool noMITShmExtension;
    Bool noRenderExtension;
    Bool noResExtension;
    Bool noRRExtension;
    Bool noScreenSaverExtension;
    Bool noXFixesExtension;
    Bool noXvExtension;
    Bool PanoramiXExtensionDisabledHack;
    int PanoramiXNumScreens;
    int PanoramiXPixHeight;
    int PanoramiXPixWidth;
    int (*PanoramiXSaveCompositeVector[9]) (ClientPtr client);
    int (*PanoramiXSaveRenderVector[RenderNumberRequests]) (ClientPtr);
    int (*PanoramiXSaveXFixesVector[XFixesNumberRequests]) (ClientPtr);
    RegionRec PanoramiXScreenRegion;
    
    // Additional global variables  
    struct { Mask mask; int type; } EventInfo[32];  // From Xi module
    int ExtEventIndex;
    int FakeScreenFps;
    DevPrivateKeyRec GEClientPrivateKeyRec;
    void* GEExtensions;  // GEExtension*
    RESTYPE GlyphSetType;
    
    // Input device error codes
    int BadDevice;
    int BadShmSegCode;
    int ChangeDeviceNotify;
    Bool DPMSEnabled;
    Bool DPMSDisabledSwitch;
    CARD16 DPMSPowerLevel;
    
    // Composite module variables
    DevPrivateKeyRec CompScreenPrivateKeyRec;
    DevPrivateKeyRec CompSubwindowsPrivateKeyRec;
    DevPrivateKeyRec CompWindowPrivateKeyRec;
    RESTYPE CompositeClientOverlayType;
    RESTYPE CompositeClientSubwindowsType;
    RESTYPE CompositeClientWindowType;
    
    // Ephyr module variables
    ScreenPtr ephyrCursorScreen;
    void* ephyrKbd;  // KdKeyboardInfo*
    void* ephyrMouse;  // KdPointerInfo*
    Bool ephyrNoDRI;
    Bool ephyrNoXV; 
    char* ephyrResName;
    int ephyrResNameFromCmd;
    char* ephyrTitle;
    void* cursorScreenDevPriv;
    Bool EphyrWantGrayScale;
    Bool EphyrWantNoHostGrab;
    Bool EphyrWantResize;
    
    // Xi (X Input) module variables
    int IReqCode;
    int IEventBase;
    int BadMode;
    int DeviceBusy;
    int BadClass;
    int DeviceValuator;
    int DeviceKeyPress;
    int DeviceKeyRelease;
    int DeviceButtonPress;
    int DeviceButtonRelease;
    int DeviceMotionNotify;
    int DeviceFocusIn;
    int DeviceFocusOut;
    int ProximityIn;
    int ProximityOut;
    int DeviceStateNotify;
    int DeviceKeyStateNotify;
    int DeviceButtonStateNotify;
    int DeviceMappingNotify;
    int DevicePresenceNotify;
    int DevicePropertyNotify;
    RESTYPE RT_INPUTCLIENT;
    DevPrivateKeyRec XIClientPrivateKeyRec;
    XExtensionVersion XIVersion;
    CARD8 event_base[7];  // numInputClasses = 7, indexed by KeyClass, ButtonClass, etc.
    
    // GLX module variables
    ExtensionEntry* GlxExtensionEntry;
    int GlxErrorBase;
    RESTYPE __glXContextRes;
    RESTYPE __glXDrawableRes;
    int __glXEventBase;
    RESTYPE idResource;
    
    // Glamor module variables
    Atom glamorBrightness;
    Atom glamorColorspace;
    Atom glamorContrast;
    Atom glamorGamma;
    Atom glamorHue;
    Atom glamorSaturation;
    int glamor_debug_level;
    DevPrivateKeyRec glamor_gc_private_key;
    DevPrivateKeyRec glamor_pixmap_private_key;
    DevPrivateKeyRec glamor_screen_private_key;
    
    // Ephyr Glamor variables
    Bool ephyr_glamor;
    Bool ephyr_glamor_gles2;
    Bool ephyr_glamor_skip_present;
    
    // XFixes module variables
    Bool CursorVisible;
    int XFixesErrorBase;
    int XFixesEventBase;
    
    // Picture/Render module variables
    RESTYPE PictFormatType;
    int PictureCmapPolicy;
    DevPrivateKeyRec PictureScreenPrivateKeyRec;
    RESTYPE PictureType;
    DevPrivateKeyRec PictureWindowPrivateKeyRec;
    RESTYPE PointerBarrierType;
    
    // RandR module variables
    DevPrivateKeyRec RRClientPrivateKeyRec;
    RESTYPE RRClientType;
    RESTYPE RRCrtcType;
    int RRErrorBase;
    int RREventBase;
    RESTYPE RREventType;
    RESTYPE RRLeaseType;
    RESTYPE RRModeType;
    RESTYPE RROutputType;
    RESTYPE RRProviderType;
    int RRGeneration;
    int RRNScreens;
    
    // Render module variables
    int RenderErrBase;
    
    // SHM module variables
    int ShmCompletionCode;
    RESTYPE ShmSegType;
    
    // Saved procedure vector
    int (*SavedProcVector[256]) (ClientPtr client);
    
    // Additional global variables  
    int present_request;
    DevPrivateKeyRec present_screen_private_key;
    DevPrivateKeyRec present_window_private_key;
    DevPrivateKeyRec rrPrivKeyRec;
    Bool noRRXineramaExtension;
    
    // Mi module variables
    DevPrivateKeyRec miPointerPrivKeyRec;
    DevPrivateKeyRec miPointerScreenKeyRec;
    DevPrivateKeyRec miSyncScreenPrivateKey;
    DevPrivateKeyRec miZeroLineScreenKeyRec;
    DevPrivateKeyRec micmapScrPrivateKeyRec;
    
    // XKB module variables
    int xkbDebugFlags;
    CARD32 xkbDebugCtrls;
    DevPrivateKeyRec xkbDevicePrivateKeyRec;
    
    // XTest module variables
    DeviceIntPtr xtestkeyboard;
    DeviceIntPtr xtestpointer;
    
    // Panoramix resource types
    RESTYPE XRC_DRAWABLE;
    RESTYPE XRT_COLORMAP;
    RESTYPE XRT_GC;
    RESTYPE XRT_PICTURE;
    RESTYPE XRT_PIXMAP;
    RESTYPE XRT_WINDOW;
    CallbackListPtr XaceHooks[15]; /* XACE_NUM_HOOKS = 15 */
    int XkbEventBase;
    int XkbKeyboardErrorCode;
    int XkbReqCode;
    
    // Xv extension variables
    int XvErrorBase;
    int XvEventBase;
    unsigned long XvExtensionGeneration;
    unsigned long XvScreenGeneration;
    unsigned long XvResourceGeneration;
    int XvReqCode;
    
    // XvMC extension variables
    int XvMCEventBase;
    int XvMCReqCode;
    void* XvMCScreenInitProc;  /* Actually: int (*)(ScreenPtr, int, XvMCAdaptorPtr) */
    
    // Xv resource types
    RESTYPE XvRTPort;
    RESTYPE XvRTEncoding;
    RESTYPE XvRTGrab;
    RESTYPE XvRTVideoNotify;
    RESTYPE XvRTVideoNotifyList;
    RESTYPE XvRTPortNotify;
    unsigned long XvXRTPort;
    
    // DBE resource types
    RESTYPE dbeDrawableResType;
    DevPrivateKeyRec dbeScreenPrivKeyRec;
    DevPrivateKeyRec dbeWindowPrivKeyRec;
    RESTYPE dbeWindowPrivResType;
    
    // EXA private keys
    DevPrivateKeyRec exaScreenPrivateKeyRec;
    
    // Additional static DevPrivateKey variables
    DevPrivateKeyRec AnimCurScreenPrivateKeyRec;
    DevPrivateKeyRec BarrierScreenPrivateKeyRec;
    DevPrivateKeyRec ClientDisconnectPrivateKeyRec;
    DevPrivateKeyRec CompositeClientPrivateKeyRec;
    DevPrivateKeyRec CursorScreenPrivateKeyRec;
    DevPrivateKeyRec DamageClientPrivateKeyRec;
    DevPrivateKeyRec KdXVScreenPrivateKey;
    DevPrivateKeyRec KdXVWindowKeyRec;
    DevPrivateKey KdXvScreenKey;
    DevPrivateKeyRec PanoramiXGCKeyRec;
    DevPrivateKeyRec PanoramiXScreenKeyRec;
    
    // KDrive (kdrive) module variables
    struct _KdCardInfo* kdCardInfo;
    Bool kdDisableZaphod;
    Bool kdEmulateMiddleButton;
    char* kdGlobalXkbLayout;
    char* kdGlobalXkbModel;
    char* kdGlobalXkbOptions;
    char* kdGlobalXkbRules;
    char* kdGlobalXkbVariant;
    Bool kdHasKbd;
    Bool kdHasPointer;
    Bool kdRawPointerCoordinates;
    DevPrivateKeyRec kdScreenPrivateKeyRec;
    Bool kdEnabled;
    int kdSubpixelOrder;
    char* kdSwitchCmd;
    DDXPointRec kdOrigin;
    Bool kdDumbDriver;
    Bool kdSoftCursor;
    
    // Callback and event static variables (Phase 2)
    void* ConnectionCallbackList;  // XineramaConnectionCallbackList*
    void* ExtensionModuleList;     // ExtensionModule*
    OsSigWrapperPtr OsSigWrapper;
    RESTYPE AttrType;              // Screen saver attributes
    RESTYPE SelectionClientType;   // XFixes selection client type
    RESTYPE SelectionWindowType;   // XFixes selection window type
    RESTYPE ClientType;            // Shape extension client type
    RESTYPE ShapeEventType;        // Shape extension event type
    CARD8 CompositeReqCode;        // Composite extension request code
    RESTYPE CursorClientType;      // XFixes cursor client type
    RESTYPE CursorHideCountType;   // XFixes cursor hide count type
    RESTYPE CursorWindowType;      // XFixes cursor window type
    int DamageEventBase;           // Damage extension event base
    RESTYPE DamageExtType;         // Damage extension type
    unsigned char DamageReqCode;   // Damage extension request code
    
    // Phase 3 static globals from various modules
    WindowPtr FocusWindows[MAXDEVICES];  // dix/enterleave.c
    EphyrHostXVars HostX;                // hw/kdrive/ephyr/hostx.c
    int HostXWantDamageDebug;           // hw/kdrive/ephyr/hostx.c
    XtransConnInfo *ListenTransConns;   // os/connection.c
    int ListenTransCount;               // os/connection.c
    int *ListenTransFds;                // os/connection.c
    enum {                              // os/access.c
        LOCAL_ACCESS_SCOPE_HOST = 0,
        LOCAL_ACCESS_SCOPE_USER,
    } LocalAccessScope;
    int LocalHostEnabled;               // os/access.c
    int LocalHostRequested;             // os/access.c
    unsigned int NumExtensions;         // dix/extension.c
    pid_t ParentProcess;                // os/connection.c
    int PictureGeneration;              // render/picture.c
    
    // Phase 4 static globals - OS and extension variables
    OsCommPtr AvailableInput;           // os/io.c
    Bool BlockHandlerRegistered;        // Xext/sleepuntil.c  
    int BlockedSignalCount;             // os/utils.c
    Bool CriticalOutputPending;         // os/io.c
    int DontPropagateRefCnts[8];        // dix/events.c (DNPMCOUNT=8)
    ConnectionInputPtr FreeInputs;      // os/io.c
    ConnectionOutputPtr FreeOutputs;    // os/io.c
    unsigned long KdXVGeneration;       // hw/kdrive/src/kxv.c
    char LockFile[PATH_MAX];            // os/utils.c
    int64_t Now;                        // Xext/sync.c
    DepthPtr PanoramiXDepths;           // Xext/panoramiX.c
    int PanoramiXNumDepths;             // Xext/panoramiX.c
    int PanoramiXNumVisuals;            // Xext/panoramiX.c
    int (*PanoramiXSaveDamageCreate) (ClientPtr);  // damageext/damageext.c
    VisualPtr PanoramiXVisuals;         // Xext/panoramiX.c
    WindowPtr PointerWindows[MAXDEVICES];  // dix/enterleave.c
    RESTYPE PortResource;               // Xext/xvmain.c
    sigset_t PreviousSignalMask;        // os/utils.c
    RESTYPE RTAlarm;                    // Xext/sync.c
    RESTYPE RTAlarmClient;              // Xext/sync.c
    RESTYPE RTAwait;                    // Xext/sync.c
    RESTYPE RTCounter;                  // Xext/sync.c
    RESTYPE RTFence;                    // Xext/sync.c
    RESTYPE RTContext;                  // dix/dispatch.c
    
} XephyrContext;


void InitGlobalsForContext(XephyrContext* context);


/* Main function with context parameter */
int dix_main(int argc, char *argv[], char *envp[], XephyrContext* context);

#endif /* DIX_CONTEXT_H */
