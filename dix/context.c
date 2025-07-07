#include "dix/context.h"
#include "dixstruct.h"
#include "os.h"

XephyrContext *xephyr_context = NULL;

void InitGlobals(void)
{
    if (xephyr_context)
        return;

    xephyr_context = calloc(1, sizeof(XephyrContext));
    if (!xephyr_context)
        return; // Or handle error more gracefully

    xephyr_context->defaultKeyboardControl = (KeybdCtrl) {
        DEFAULT_KEYBOARD_CLICK,
        DEFAULT_BELL,
        DEFAULT_BELL_PITCH,
        DEFAULT_BELL_DURATION,
        DEFAULT_AUTOREPEAT,
        DEFAULT_AUTOREPEATS,
        DEFAULT_LEDS,
        0
    };
    xephyr_context->defaultPointerControl = (PtrCtrl) {
        DEFAULT_PTR_NUMERATOR,
        DEFAULT_PTR_DENOMINATOR,
        DEFAULT_PTR_THRESHOLD,
        0
    };
    xephyr_context->maxBigRequestSize = MAX_BIG_REQUEST_SIZE;
    xephyr_context->globalSerialNumber = 0;
    xephyr_context->serverGeneration = 0;
    xephyr_context->defaultScreenSaverTime = (10 * (60 * 1000));
    xephyr_context->defaultScreenSaverInterval = (10 * (60 * 1000));
    xephyr_context->defaultScreenSaverBlanking = PreferBlanking;
    xephyr_context->defaultScreenSaverAllowExposures = AllowExposures;
    xephyr_context->screenSaverSuspended = FALSE;
    xephyr_context->defaultFontPath = "/usr/share/fonts";
    xephyr_context->party_like_its_1989 = FALSE;
    xephyr_context->whiteRoot = FALSE;
    xephyr_context->defaultColorVisualClass = -1;
    xephyr_context->monitorResolution = 0;
    xephyr_context->displayfd = -1;
    xephyr_context->explicit_display = FALSE;
    
    // Initialize OS/backing store variables
    xephyr_context->enableBackingStore = FALSE;
    xephyr_context->disableBackingStore = FALSE;
    
    // Initialize resource limit variables
    xephyr_context->limitDataSpace = 0;
    xephyr_context->limitNoFile = 0;
    xephyr_context->limitStackSpace = 0;
    
    // Initialize background/window variables
    xephyr_context->bgNoneRoot = FALSE;
    
    // Initialize process control variables
    xephyr_context->RunFromSigStopParent = FALSE;
    
    // Initialize access control variables
    xephyr_context->defeatAccessControl = FALSE;
    
    // Initialize network variables
    xephyr_context->PartialNetwork = TRUE;
    
    // Initialize signal handling variables
    xephyr_context->inSignalContext = FALSE;
    
    // Initialize extension control variables
    xephyr_context->noTestExtensions = FALSE;
    
    // Initialize seat configuration  
    xephyr_context->SeatId = NULL;
    
    // Initialize GLX configuration
    xephyr_context->enableIndirectGLX = FALSE;

}
