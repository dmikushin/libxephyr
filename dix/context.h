
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
#include <sys/select.h>
#include "opaque.h"
#include <X11/Xproto.h>
#include "../os/ospoll.h"
#include "servermd.h"
#include "privates.h"
#include "resource.h"
#ifdef CONFIG_UDEV
#include <libudev.h>
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
    int dispatchException;
    OsTimerPtr dispatchExceptionTimer;
    Bool isItTimeToYield;
    fd_set output_pending_clients;
    PaddingInfo PixmapWidthPaddingInfo[33];
    CallbackListPtr ServerGrabCallback;
    Bool SmartScheduleLatencyLimited;
    CARD32 SmartScheduleTime;
    int terminateDelay;
    void* fontPatternCache;
    CallbackListPtr RootWindowFinalizeCallback;
    void* lastGLContext;
    WorkQueuePtr workQueue;
    CallbackListPtr DeviceEventCallback;
    Mask DontPropagateMasks[MAXDEVICES];
    CallbackListPtr EventCallback;
    InternalEvent *event_filters[128];
    Bool syncEvents;
    InternalEvent *InputEventList;
    CallbackListPtr PropertyStateCallback;
    RegionRec RegionBrokenData;
    BoxRec RegionEmptyBox;
    RegionRec RegionEmptyData;
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
    void* PanoramiXSaveCompositeVector;
    void* PanoramiXSaveRenderVector;
    void* PanoramiXSaveXFixesVector;
    RegionPtr PanoramiXScreenRegion;
    
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
    char* ephyrResNameFromCmd;
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
    
    // GLX module variables
    ExtensionEntry* GlxExtensionEntry;
    int GlxErrorBase;
    RESTYPE __glXContextRes;
    RESTYPE __glXDrawableRes;
    int __glXEventBase;
    
    // Glamor module variables
    Atom glamorBrightness;
    Atom glamorColorspace;
    Atom glamorContrast;
    Atom glamorGamma;
    Atom glamorHue;
    Atom glamorSaturation;
    int glamor_debug_level;
    
    // Ephyr Glamor variables
    Bool ephyr_glamor;
    Bool ephyr_glamor_gles2;
    Bool ephyr_glamor_skip_present;
    
    // XFixes module variables
    Bool CursorVisible;
    
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
    int RREventType;
    RESTYPE RRLeaseType;
    RESTYPE RRModeType;
    RESTYPE RROutputType;
    RESTYPE RRProviderType;
    RESTYPE RegionResType;
    
    // Render module variables
    int RenderErrBase;
    
    // SHM module variables
    int ShmCompletionCode;
    RESTYPE ShmSegType;
    
    // Saved procedure vector
    void* SavedProcVector;
    
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
    DevPrivateKeyRec xkbDevicePrivateKeyRec;
    
    // XTest module variables
    DeviceIntPtr xtestkeyboard;
    DeviceIntPtr xtestpointer;
} XephyrContext;


void InitGlobalsForContext(XephyrContext* context);


/* Main function with context parameter */
int dix_main(int argc, char *argv[], char *envp[], XephyrContext* context);

#endif /* DIX_CONTEXT_H */
