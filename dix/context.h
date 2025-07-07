
#ifndef DIX_CONTEXT_H
#define DIX_CONTEXT_H

#include "dix-config.h"
#include <X11/X.h>
#include <X11/Xmd.h>
#include "dixstruct.h"
#include "scrnintstr.h"
#include "input.h"
#include "client.h"
#include <X11/fonts/font.h>
#include "cursor.h"
#include "misc.h"

typedef struct _XephyrContext {
    ScreenInfo screenInfo;
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
    
    // GLX configuration
    Bool enableIndirectGLX;
} XephyrContext;

extern XephyrContext *xephyr_context;

void InitGlobals(void);

#endif /* DIX_CONTEXT_H */
