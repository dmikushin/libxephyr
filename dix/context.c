#include "dix/context.h"
#include "dixstruct.h"
#include "os.h"

void InitGlobalsForContext(XephyrContext* context)
{
    if (!context)
        return;

    context->defaultKeyboardControl = (KeybdCtrl) {
        DEFAULT_KEYBOARD_CLICK,
        DEFAULT_BELL,
        DEFAULT_BELL_PITCH,
        DEFAULT_BELL_DURATION,
        DEFAULT_AUTOREPEAT,
        DEFAULT_AUTOREPEATS,
        DEFAULT_LEDS,
        0
    };
    context->defaultPointerControl = (PtrCtrl) {
        DEFAULT_PTR_NUMERATOR,
        DEFAULT_PTR_DENOMINATOR,
        DEFAULT_PTR_THRESHOLD,
        0
    };
    context->maxBigRequestSize = MAX_BIG_REQUEST_SIZE;
    context->globalSerialNumber = 0;
    context->serverGeneration = 0;
    context->defaultScreenSaverTime = (10 * (60 * 1000));
    context->defaultScreenSaverInterval = (10 * (60 * 1000));
    context->defaultScreenSaverBlanking = PreferBlanking;
    context->defaultScreenSaverAllowExposures = AllowExposures;
    context->screenSaverSuspended = FALSE;
    context->defaultFontPath = "/usr/share/fonts";
    context->party_like_its_1989 = FALSE;
    context->whiteRoot = FALSE;
    context->defaultColorVisualClass = -1;
    context->monitorResolution = 0;
    context->displayfd = -1;
    context->explicit_display = FALSE;
    
    // Initialize OS/backing store variables
    context->enableBackingStore = FALSE;
    context->disableBackingStore = FALSE;
    
    // Initialize resource limit variables
    context->limitDataSpace = 0;
    context->limitNoFile = 0;
    context->limitStackSpace = 0;
    
    // Initialize background/window variables
    context->bgNoneRoot = FALSE;
    
    // Initialize process control variables
    context->RunFromSigStopParent = FALSE;
    
    // Initialize access control variables
    context->defeatAccessControl = FALSE;
    
    // Initialize network variables
    context->PartialNetwork = TRUE;
    
    // Initialize signal handling variables
    context->inSignalContext = FALSE;
    
    // Initialize extension control variables
    context->noTestExtensions = FALSE;
    
    // Initialize seat configuration  
    context->SeatId = NULL;
    
    // Initialize GLX configuration
    context->enableIndirectGLX = FALSE;
    
    // Initialize work queue
    context->workQueue = NULL;
    
    // Initialize input device error codes
    context->BadDevice = 0; // Will be set later when XI extension loads
    context->BadShmSegCode = 0; // Will be set later when SHM extension loads
    context->ChangeDeviceNotify = 0; // Will be set later when XI extension loads
    context->DPMSEnabled = FALSE;
    
    // Initialize Xi extension codes
    context->IReqCode = 0;
    context->IEventBase = 0;
    context->BadMode = 2;
    context->DeviceBusy = 3;
    context->BadClass = 4;
}

