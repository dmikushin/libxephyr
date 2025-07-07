include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)

# Configuration data
set(conf_data "")

# Always enable GLX and Glamor for libxephyr
set(GLXEXT 1)
set(GLAMOR 1)

# Basic configuration for dix
set(_DIX_CONFIG_H_ 1)
test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    set(X_BYTE_ORDER "X_BIG_ENDIAN")
else()
    set(X_BYTE_ORDER "X_LITTLE_ENDIAN")
endif()

# For libxephyr, we need _XSERVER64 to make XID = CARD32 (4 bytes)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
if(SIZEOF_UNSIGNED_LONG EQUAL 8)
    set(_XSERVER64 1)
endif()

set(_GNU_SOURCE 1)

# Check for functions
check_function_exists(reallocarray HAVE_REALLOCARRAY)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strcasestr HAVE_STRCASESTR)
check_function_exists(strlcat HAVE_STRLCAT)
check_function_exists(strlcpy HAVE_STRLCPY)
check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(timingsafe_memcmp HAVE_TIMINGSAFE_MEMCMP)
check_function_exists(poll HAVE_POLL)
check_function_exists(sigaction BUSFAULT)
check_function_exists(getpeereid HAVE_GETPEEREID)
check_function_exists(getpeerucred HAVE_GETPEERUCRED)

# Check for pthread
check_symbol_exists(PTHREAD_MUTEX_RECURSIVE "pthread.h" HAVE_PTHREAD_MUTEX_RECURSIVE)
if(HAVE_PTHREAD_MUTEX_RECURSIVE)
    set(INPUTTHREAD 1)
endif()

# Check for peer credentials
if(NOT HAVE_GETPEEREID AND NOT HAVE_GETPEERUCRED)
    check_symbol_exists(SO_PEERCRED "sys/socket.h" HAVE_SO_PEERCRED)
    if(NOT HAVE_SO_PEERCRED)
        set(NO_LOCAL_CLIENT_CRED 1)
    endif()
endif()

# Set hardcoded dependencies
if(XSHMFENCE_FOUND)
    set(HAVE_XSHMFENCE 1)
endif()

if(LIBDRM_FOUND)
    set(WITH_LIBDRM 1)
endif()

if(GBM_FOUND)
    set(GLAMOR_HAS_GBM 1)
endif()

# Set default font path - standard X11 font directories
set(COMPILEDDEFAULTFONTPATH "/usr/share/fonts/X11/misc/,/usr/share/fonts/X11/TTF/,/usr/share/fonts/X11/OTF/,/usr/share/fonts/X11/Type1/,/usr/share/fonts/X11/100dpi/,/usr/share/fonts/X11/75dpi/")

# Transport configuration - set defaults for libxephyr
set(LISTEN_TCP OFF CACHE BOOL "Enable TCP transport")
set(LISTEN_UNIX ON CACHE BOOL "Enable Unix transport")
set(LISTEN_LOCAL OFF CACHE BOOL "Enable local transport")

# Xtrans transport protocol defines
if(LISTEN_TCP)
    set(TCPCONN 1)
endif()
if(LISTEN_UNIX)
    set(UNIXCONN 1)
endif()

# Hardcode some feature flags
set(COMPOSITE 1)
set(DAMAGE 1)
set(DBE 1)
set(DRI2 1)
set(DRI3 1)
set(HAS_SHM 1)
set(MITSHM 1)
set(PANORAMIX 1)
set(PRESENT 1)
set(RANDR 1)
set(RENDER 1)
set(SCREENSAVER 1)
set(SHAPE 1)
set(XACE 1)
set(XFIXES 1)
set(XINERAMA 1)
set(XINPUT 1)
set(XRECORD 1)
set(XSYNC 1)
set(XTEST 1)
set(XV 1)
set(XvExtension 1)

# Configure dix-config.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/dix-config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dix-config.h"
    @ONLY
)

# Configure xkb-config.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/xkb-config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/xkb-config.h"
    @ONLY
)

# Add the binary directory to include path for generated headers
include_directories(${CMAKE_CURRENT_BINARY_DIR})
