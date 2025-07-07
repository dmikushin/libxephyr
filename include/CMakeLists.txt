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

# Create the template files if they don't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dix-config.h.in")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/dix-config.h.in"
"#ifndef _DIX_CONFIG_H_
#define _DIX_CONFIG_H_

#cmakedefine _DIX_CONFIG_H_ 1
#cmakedefine _XSERVER64 1
#cmakedefine _GNU_SOURCE 1
#cmakedefine GLXEXT 1
#cmakedefine GLAMOR 1

#cmakedefine X_BYTE_ORDER @X_BYTE_ORDER@

#cmakedefine HAVE_REALLOCARRAY 1
#cmakedefine HAVE_STRCASECMP 1
#cmakedefine HAVE_STRCASESTR 1
#cmakedefine HAVE_STRLCAT 1
#cmakedefine HAVE_STRLCPY 1
#cmakedefine HAVE_STRNDUP 1
#cmakedefine HAVE_TIMINGSAFE_MEMCMP 1
#cmakedefine HAVE_POLL 1
#cmakedefine BUSFAULT 1
#cmakedefine INPUTTHREAD 1
#cmakedefine HAVE_GETPEEREID 1
#cmakedefine HAVE_GETPEERUCRED 1
#cmakedefine NO_LOCAL_CLIENT_CRED 1

#cmakedefine HAVE_XSHMFENCE 1
#cmakedefine WITH_LIBDRM 1
#cmakedefine GLAMOR_HAS_GBM 1

#cmakedefine COMPILEDDEFAULTFONTPATH \"@COMPILEDDEFAULTFONTPATH@\"

#cmakedefine LISTEN_TCP 1
#cmakedefine LISTEN_UNIX 1
#cmakedefine LISTEN_LOCAL 1
#cmakedefine TCPCONN 1
#cmakedefine UNIXCONN 1

#cmakedefine COMPOSITE 1
#cmakedefine DAMAGE 1
#cmakedefine DBE 1
#cmakedefine DRI2 1
#cmakedefine DRI3 1
#cmakedefine HAS_SHM 1
#cmakedefine MITSHM 1
#cmakedefine PANORAMIX 1
#cmakedefine PRESENT 1
#cmakedefine RANDR 1
#cmakedefine RENDER 1
#cmakedefine SCREENSAVER 1
#cmakedefine SHAPE 1
#cmakedefine XACE 1
#cmakedefine XFIXES 1
#cmakedefine XINERAMA 1
#cmakedefine XINPUT 1
#cmakedefine XRECORD 1
#cmakedefine XSYNC 1
#cmakedefine XTEST 1
#cmakedefine XV 1
#cmakedefine XvExtension 1

#endif /* _DIX_CONFIG_H_ */
")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xkb-config.h.in")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/xkb-config.h.in"
"#ifndef _XKB_CONFIG_H_
#define _XKB_CONFIG_H_

#cmakedefine XKB_BASE_DIRECTORY \"@XKB_DIR@\"
#cmakedefine XKB_BIN_DIRECTORY \"@XKB_BIN_DIR@\"
#cmakedefine XKB_COMPILED_DIR \"@XKB_OUTPUT_DIR@\"
#cmakedefine XKM_OUTPUT_DIR \"@XKB_OUTPUT_DIR@\"
#cmakedefine XKB_DFLT_RULES_FILE \"base\"
#cmakedefine XKB_DFLT_KB_LAYOUT \"us\"
#cmakedefine XKB_DFLT_KB_MODEL \"pc105\"
#cmakedefine XKB_DFLT_KB_VARIANT \"\"
#cmakedefine XKB_DFLT_KB_OPTIONS \"\"

#endif /* _XKB_CONFIG_H_ */
")
endif()

# Add the binary directory to include path for generated headers
include_directories(${CMAKE_CURRENT_BINARY_DIR})