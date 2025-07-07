cmake_minimum_required(VERSION 3.16)
project(libxephyr VERSION 21.1.4 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Enable position independent code for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add compile definitions
add_definitions(-DHAVE_DIX_CONFIG_H)

# Find required packages
find_package(PkgConfig REQUIRED)

# Basic dependencies needed for libxephyr
pkg_check_modules(XPROTO REQUIRED xproto>=7.0.31)
pkg_check_modules(RANDRPROTO REQUIRED randrproto>=1.6.0)
pkg_check_modules(RENDERPROTO REQUIRED renderproto>=0.11)
pkg_check_modules(XEXTPROTO REQUIRED xextproto>=7.2.99.901)
pkg_check_modules(INPUTPROTO REQUIRED inputproto>=2.3.99.1)
pkg_check_modules(KBPROTO REQUIRED kbproto>=1.0.3)
pkg_check_modules(FONTSPROTO REQUIRED fontsproto>=2.1.3)
pkg_check_modules(FIXESPROTO REQUIRED fixesproto>=6.0)
pkg_check_modules(DAMAGEPROTO REQUIRED damageproto>=1.1)
pkg_check_modules(XCMISCPROTO REQUIRED xcmiscproto>=1.2.0)
pkg_check_modules(BIGREQSPROTO REQUIRED bigreqsproto>=1.1.0)
pkg_check_modules(XTRANS REQUIRED xtrans>=1.3.5)
pkg_check_modules(VIDEOPROTO REQUIRED videoproto)
pkg_check_modules(COMPOSITEPROTO REQUIRED compositeproto>=0.4)
pkg_check_modules(RECORDPROTO REQUIRED recordproto>=1.13.99.1)
pkg_check_modules(SCRNSAVERPROTO REQUIRED scrnsaverproto>=1.1)
pkg_check_modules(RESOURCEPROTO REQUIRED resourceproto>=1.2.0)
pkg_check_modules(DRI2PROTO REQUIRED dri2proto>=2.8)
pkg_check_modules(DRI3PROTO REQUIRED dri3proto>=1.2)
pkg_check_modules(XINERAMAPROTO REQUIRED xineramaproto)
pkg_check_modules(XSHMFENCE xshmfence>=1.1)
pkg_check_modules(PIXMAN REQUIRED pixman-1)
pkg_check_modules(LIBBSD libbsd)
pkg_check_modules(XKBFILE REQUIRED xkbfile)
pkg_check_modules(XFONT2 REQUIRED xfont2>=2.0)
pkg_check_modules(DBUS dbus-1>=1.0)
pkg_check_modules(LIBDRM libdrm)
pkg_check_modules(EPOXY epoxy)
pkg_check_modules(GBM gbm)
pkg_check_modules(HAL hal)
pkg_check_modules(UDEV libudev)
pkg_check_modules(XAU REQUIRED xau)

# GLX dependencies
pkg_check_modules(GLPROTO REQUIRED glproto>=1.4.17)
pkg_check_modules(GL REQUIRED gl>=1.2)

# Check for DRI and get DRI driver path
pkg_check_modules(DRI dri)
if(DRI_FOUND)
    execute_process(
        COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=dridriverdir dri
        OUTPUT_VARIABLE DRI_DRIVER_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(DRI_DRIVER_PATH "")
endif()

# Additional system libraries
find_library(UNWIND_LIBRARY unwind)
find_library(BSD_LIBRARY bsd)

# Check for libbsd for timingsafe_memcmp
# check_library_exists(bsd timingsafe_memcmp "" HAVE_LIBBSD_TIMINGSAFE)

# Disable DRI3 support
set(DRI3 0)

# XCB dependencies for Xephyr
pkg_check_modules(XCB REQUIRED xcb)
pkg_check_modules(XCB_SHAPE REQUIRED xcb-shape)
pkg_check_modules(XCB_RENDER REQUIRED xcb-render)
pkg_check_modules(XCB_RENDERUTIL REQUIRED xcb-renderutil)
pkg_check_modules(XCB_AUX REQUIRED xcb-aux)
pkg_check_modules(XCB_IMAGE REQUIRED xcb-image)
pkg_check_modules(XCB_ICCCM REQUIRED xcb-icccm)
pkg_check_modules(XCB_SHM REQUIRED xcb-shm>=1.9.3)
pkg_check_modules(XCB_KEYSYMS REQUIRED xcb-keysyms)
pkg_check_modules(XCB_RANDR REQUIRED xcb-randr)
pkg_check_modules(XCB_XKB REQUIRED xcb-xkb)
pkg_check_modules(X11_XCB REQUIRED x11-xcb)
pkg_check_modules(XCB_XV REQUIRED xcb-xv)

# Find system libraries
find_library(M_LIBRARY m)
find_library(DL_LIBRARY dl)

# Feature testing for dix-config.h
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(TestBigEndian)

# Header file checks
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(execinfo.h HAVE_EXECINFO_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(fnmatch.h HAVE_FNMATCH_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(linux/agpgart.h HAVE_LINUX_AGPGART_H)
check_include_file(linux/apm_bios.h HAVE_LINUX_APM_BIOS_H)
check_include_file(linux/fb.h HAVE_LINUX_FB_H)
check_include_file(ndir.h HAVE_NDIR_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/agpio.h HAVE_SYS_AGPIO_H)
check_include_file(sys/dir.h HAVE_SYS_DIR_H)
check_include_file(sys/ndir.h HAVE_SYS_NDIR_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/sysmacros.h HAVE_SYS_SYSMACROS_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/utsname.h HAVE_SYS_UTSNAME_H)
check_include_file(unistd.h HAVE_UNISTD_H)

# Function checks
check_function_exists(arc4random_buf HAVE_ARC4RANDOM_BUF)
check_function_exists(backtrace HAVE_BACKTRACE)
check_function_exists(cbrt HAVE_CBRT)
check_function_exists(epoll_create1 HAVE_EPOLL_CREATE1)
check_function_exists(getdtablesize HAVE_GETDTABLESIZE)
check_function_exists(getifaddrs HAVE_GETIFADDRS)
check_function_exists(getpeereid HAVE_GETPEEREID)
check_function_exists(getpeerucred HAVE_GETPEERUCRED)
check_function_exists(getprogname HAVE_GETPROGNAME)
check_function_exists(getzoneid HAVE_GETZONEID)
check_function_exists(isastream HAVE_ISASTREAM)
check_function_exists(memfd_create HAVE_MEMFD_CREATE)
check_function_exists(mkostemp HAVE_MKOSTEMP)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(poll HAVE_POLL)
check_function_exists(posix_fallocate HAVE_POSIX_FALLOCATE)
check_function_exists(reallocarray HAVE_REALLOCARRAY)
check_function_exists(setitimer HAVE_SETITIMER)
check_function_exists(shmctl64 HAVE_SHMCTL64)
check_function_exists(sigprocmask HAVE_SIGPROCMASK)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strcasestr HAVE_STRCASESTR)
check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(strlcat HAVE_STRLCAT)
check_function_exists(strlcpy HAVE_STRLCPY)
check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(timingsafe_memcmp HAVE_TIMINGSAFE_MEMCMP)
check_function_exists(vasprintf HAVE_VASPRINTF)

# Library checks
check_library_exists(bsd reallocarray "" HAVE_LIBBSD)
check_library_exists(m cbrt "" HAVE_LIBM)
check_library_exists(unwind backtrace "" HAVE_LIBUNWIND)

# BSD-specific checks
if(LIBBSD_FOUND)
    set(CMAKE_REQUIRED_LIBRARIES ${LIBBSD_LIBRARIES})
    check_function_exists(reallocarray HAVE_REALLOCARRAY)
    check_function_exists(strlcat HAVE_STRLCAT)
    check_function_exists(strlcpy HAVE_STRLCPY)
    set(CMAKE_REQUIRED_LIBRARIES)
endif()

# Symbol checks
check_symbol_exists(program_invocation_short_name "errno.h" HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME)
check_symbol_exists(SO_PEERCRED "sys/socket.h" HAVE_SO_PEERCRED)
check_symbol_exists(PTHREAD_MUTEX_RECURSIVE "pthread.h" HAVE_PTHREAD_MUTEX_RECURSIVE)

# Pthread setname checks
check_function_exists(pthread_setname_np HAVE_PTHREAD_SETNAME_NP)
if(HAVE_PTHREAD_SETNAME_NP)
    # Check if it takes one argument (thread name only)
    check_c_source_compiles(
        "#include <pthread.h>
         int main() { pthread_setname_np(\"test\"); return 0; }"
        HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID)
    if(NOT HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID)
        set(HAVE_PTHREAD_SETNAME_NP_WITH_TID 1)
    endif()
endif()

# SHA1 implementation checks
check_function_exists(CC_SHA1_Init HAVE_SHA1_IN_COMMONCRYPTO)
check_function_exists(SHA1Init HAVE_SHA1_IN_LIBC)
check_library_exists(md SHA1Init "" HAVE_SHA1_IN_LIBMD)
check_library_exists(gcrypt gcry_md_open "" HAVE_SHA1_IN_LIBGCRYPT)
check_library_exists(nettle sha1_init "" HAVE_SHA1_IN_LIBNETTLE)
check_library_exists(sha1 sha1_begin "" HAVE_SHA1_IN_LIBSHA1)

# Endianness check
test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    set(X_BYTE_ORDER "X_BIG_ENDIAN")
else()
    set(X_BYTE_ORDER "X_LITTLE_ENDIAN")
endif()

# Architecture checks
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_XSERVER64 1)
endif()

# System-specific checks
if(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(CSRG_BASED 1)
    set(BSD44SOCKETS 1)
endif()

# Transport checks
set(TCPCONN 1)
set(UNIXCONN 1)
set(LISTEN_TCP 1)
set(LISTEN_UNIX 1)

# No local client credentials if neither getpeereid nor getpeerucred and no SO_PEERCRED
if(NOT HAVE_GETPEEREID AND NOT HAVE_GETPEERUCRED AND NOT HAVE_SO_PEERCRED)
    set(NO_LOCAL_CLIENT_CRED 1)
endif()

# Threading support
if(HAVE_PTHREAD_MUTEX_RECURSIVE)
    set(INPUTTHREAD 1)
endif()

# Check for urandom
if(EXISTS "/dev/urandom")
    set(HAVE_URANDOM 1)
endif()

# Check for SYSV IPC
check_c_source_compiles(
    "#include <sys/ipc.h>
     #include <sys/shm.h>
     int main() { shmget(IPC_PRIVATE, 1, IPC_CREAT | 0600); return 0; }"
    HAVE_SYSV_IPC)

# Busfault support (requires sigaction)
check_function_exists(sigaction HAVE_SIGACTION)
if(HAVE_SIGACTION)
    set(BUSFAULT 1)
endif()

# systemd support
if(DBUS_FOUND)
    set(HAVE_DBUS 1)
endif()

# systemd daemon support
pkg_check_modules(SYSTEMD_DAEMON libsystemd-daemon)
if(SYSTEMD_DAEMON_FOUND)
    set(HAVE_SYSTEMD_DAEMON 1)
endif()

# udev support
if(UDEV_FOUND)
    set(CMAKE_REQUIRED_LIBRARIES ${UDEV_LIBRARIES})
    check_function_exists(udev_monitor_filter_add_match_tag HAVE_UDEV_MONITOR_FILTER_ADD_MATCH_TAG)
    check_function_exists(udev_enumerate_add_match_tag HAVE_UDEV_ENUMERATE_ADD_MATCH_TAG)
    set(CMAKE_REQUIRED_LIBRARIES)
endif()

# xshmfence support
if(XSHMFENCE_FOUND)
    set(HAVE_XSHMFENCE 1)
endif()

# Check for monotonic clock
check_c_source_compiles(
    "#include <time.h>
     int main() {
         struct timespec ts;
         clock_gettime(CLOCK_MONOTONIC, &ts);
         return 0;
     }"
    MONOTONIC_CLOCK)

# Check for typeof support
check_c_source_compiles(
    "int main() { typeof(1) x = 1; return 0; }"
    HAVE_TYPEOF)

# Check for __typeof__ as fallback
if(NOT HAVE_TYPEOF)
    check_c_source_compiles(
        "int main() { __typeof__(1) x = 1; return 0; }"
        HAVE___TYPEOF__)
    if(HAVE___TYPEOF__)
        set(typeof "__typeof__")
    endif()
endif()

# Feature flags that are always enabled for libxephyr
set(COMPOSITE 1)
set(DAMAGE 1)
set(DBE 1)
set(DRI2 1)
set(DRI3 0)
set(GLAMOR 1)
set(GLXEXT 1)
set(HAS_SHM 1)
set(MITSHM 1)
set(PANORAMIX 1)
set(PRESENT 1)
set(RANDR 1)
set(RENDER 1)
set(SCREENSAVER 1)
set(SHAPE 1)
set(XACE 1)
set(XCMISC 1)
set(XFIXES 1)
set(XINERAMA 1)
set(XINPUT 1)
set(XRECORD 1)
set(XSYNC 1)
set(XTEST 1)
set(XV 1)
set(XvExtension 1)
set(BIGREQS 1)
set(RES 1)
set(CLIENTIDS 1)

# GNU source for glibc extensions
set(_GNU_SOURCE 1)

# Default systemd seat ID
set(SeatId "seat0")

# Default font path
set(COMPILEDDEFAULTFONTPATH "/usr/share/fonts/X11/misc/,/usr/share/fonts/X11/TTF/,/usr/share/fonts/X11/OTF/,/usr/share/fonts/X11/Type1/,/usr/share/fonts/X11/100dpi/,/usr/share/fonts/X11/75dpi/")

# Set XKB paths and configuration variables
set(XKB_DIR "/usr/share/X11/xkb" CACHE PATH "XKB directory")
set(XKB_OUTPUT_DIR "${XKB_DIR}/compiled" CACHE PATH "XKB output directory")
set(XKB_BIN_DIR "/usr/bin" CACHE PATH "XKB bin directory")

# XKB configuration variables for xkb-config.h
set(XKB_BASE_DIRECTORY "${XKB_DIR}")
set(XKB_BIN_DIRECTORY "${XKB_BIN_DIR}")
set(XKM_OUTPUT_DIR "${XKB_OUTPUT_DIR}")
set(XKB_DFLT_RULES "base")
set(XKB_DFLT_MODEL "pc105")
set(XKB_DFLT_LAYOUT "us")
set(XKB_DFLT_VARIANT " ")
set(XKB_DFLT_OPTIONS " ")

# XKB legacy defines (for xkbInit.c compatibility)
set(XKB_DFLT_RULES_FILE "${XKB_DFLT_RULES}")
set(XKB_DFLT_KB_MODEL "${XKB_DFLT_MODEL}")
set(XKB_DFLT_KB_LAYOUT "${XKB_DFLT_LAYOUT}")
set(XKB_DFLT_KB_VARIANT "${XKB_DFLT_VARIANT}")
set(XKB_DFLT_KB_OPTIONS "${XKB_DFLT_OPTIONS}")

# Include directories
include_directories(
    .
    Xext
    Xi
    composite
    damageext
    fb
    glamor
    mi
    miext/damage
    miext/shadow
    miext/sync
    dbe
    include
    present
    randr
    render
    xfixes
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Include must come first, as it sets up dix-config.h
add_subdirectory(include)

# Core X server components needed for Xephyr
add_subdirectory(dix)
add_subdirectory(fb)
add_subdirectory(mi)
add_subdirectory(os)

# Essential extensions for Xephyr
add_subdirectory(miext/damage)
add_subdirectory(miext/shadow)
add_subdirectory(miext/sync)
add_subdirectory(randr)
add_subdirectory(render)
add_subdirectory(xfixes)
add_subdirectory(xkb)
add_subdirectory(Xext)
add_subdirectory(Xi)
add_subdirectory(glx)
add_subdirectory(glamor)
add_subdirectory(exa)
add_subdirectory(present)
add_subdirectory(composite)
add_subdirectory(damageext)
add_subdirectory(dbe)
add_subdirectory(record)
add_subdirectory(config)
add_subdirectory(hw)

# Create the main libxephyr static library
add_library(xephyr STATIC
    $<TARGET_OBJECTS:libxserver_mi>
    $<TARGET_OBJECTS:libxserver_dix>
    $<TARGET_OBJECTS:libxserver_fb>
    $<TARGET_OBJECTS:libxserver_os>
    $<TARGET_OBJECTS:libxserver_miext_damage>
    $<TARGET_OBJECTS:libxserver_miext_shadow>
    $<TARGET_OBJECTS:libxserver_miext_sync>
    $<TARGET_OBJECTS:libxserver_randr>
    $<TARGET_OBJECTS:libxserver_render>
    $<TARGET_OBJECTS:libxserver_xfixes>
    $<TARGET_OBJECTS:libxserver_xkb>
    $<TARGET_OBJECTS:libxserver_xkb_stubs>
    $<TARGET_OBJECTS:libxserver_xext>
    $<TARGET_OBJECTS:libxserver_xi>
    $<TARGET_OBJECTS:libxserver_composite>
    $<TARGET_OBJECTS:libxserver_damageext>
    $<TARGET_OBJECTS:libxserver_dbe>
    $<TARGET_OBJECTS:libxserver_present>
    $<TARGET_OBJECTS:libxserver_record>
    $<TARGET_OBJECTS:libxserver_config>
    $<TARGET_OBJECTS:libxserver_exa>
    $<TARGET_OBJECTS:libxserver_glx>
    $<TARGET_OBJECTS:libglxvnd>
    $<TARGET_OBJECTS:glamor>
    $<TARGET_OBJECTS:glamor_egl_stubs>
    $<TARGET_OBJECTS:kdrive>
    $<TARGET_OBJECTS:ephyr_lib>
)

# Set visibility
target_compile_options(xephyr PRIVATE -fvisibility=hidden)

# Link with common dependencies
target_link_libraries(xephyr
    ${XPROTO_LIBRARIES}
    ${RANDRPROTO_LIBRARIES}
    ${RENDERPROTO_LIBRARIES}
    ${XEXTPROTO_LIBRARIES}
    ${INPUTPROTO_LIBRARIES}
    ${KBPROTO_LIBRARIES}
    ${FONTSPROTO_LIBRARIES}
    ${FIXESPROTO_LIBRARIES}
    ${DAMAGEPROTO_LIBRARIES}
    ${XCMISCPROTO_LIBRARIES}
    ${BIGREQSPROTO_LIBRARIES}
    ${XTRANS_LIBRARIES}
    ${VIDEOPROTO_LIBRARIES}
    ${COMPOSITEPROTO_LIBRARIES}
    ${RECORDPROTO_LIBRARIES}
    ${SCRNSAVERPROTO_LIBRARIES}
    ${RESOURCEPROTO_LIBRARIES}
    ${DRI2PROTO_LIBRARIES}
    ${DRI3PROTO_LIBRARIES}
    ${XINERAMAPROTO_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${XKBFILE_LIBRARIES}
    ${XFONT2_LIBRARIES}
    ${XAU_LIBRARIES}
    ${GL_LIBRARIES}
    ${M_LIBRARY}
    ${DL_LIBRARY}
)

# Add system libraries if found
if(UNWIND_LIBRARY)
    target_link_libraries(xephyr ${UNWIND_LIBRARY})
endif()

if(BSD_LIBRARY)
    target_link_libraries(xephyr ${BSD_LIBRARY})
endif()

# GLX is built from sources, no external library needed

# Add optional dependencies
if(XSHMFENCE_FOUND)
    target_link_libraries(xephyr ${XSHMFENCE_LIBRARIES})
endif()

if(LIBBSD_FOUND)
    target_link_libraries(xephyr ${LIBBSD_LIBRARIES})
endif()

# Add XCB libraries for Xephyr hardware support
target_link_libraries(xephyr
    ${XCB_LIBRARIES}
    ${XCB_SHAPE_LIBRARIES}
    ${XCB_RENDER_LIBRARIES}
    ${XCB_RENDERUTIL_LIBRARIES}
    ${XCB_AUX_LIBRARIES}
    ${XCB_IMAGE_LIBRARIES}
    ${XCB_ICCCM_LIBRARIES}
    ${XCB_SHM_LIBRARIES}
    ${XCB_KEYSYMS_LIBRARIES}
    ${XCB_RANDR_LIBRARIES}
    ${XCB_XKB_LIBRARIES}
    ${X11_XCB_LIBRARIES}
    ${XCB_XV_LIBRARIES}
)

if(EPOXY_FOUND)
    target_link_libraries(xephyr ${EPOXY_LIBRARIES})
endif()

# Add compile flags for all dependencies
target_compile_options(xephyr PRIVATE
    ${XPROTO_CFLAGS}
    ${RANDRPROTO_CFLAGS}
    ${RENDERPROTO_CFLAGS}
    ${XEXTPROTO_CFLAGS}
    ${INPUTPROTO_CFLAGS}
    ${KBPROTO_CFLAGS}
    ${FONTSPROTO_CFLAGS}
    ${FIXESPROTO_CFLAGS}
    ${DAMAGEPROTO_CFLAGS}
    ${XCMISCPROTO_CFLAGS}
    ${BIGREQSPROTO_CFLAGS}
    ${XTRANS_CFLAGS}
    ${VIDEOPROTO_CFLAGS}
    ${COMPOSITEPROTO_CFLAGS}
    ${RECORDPROTO_CFLAGS}
    ${SCRNSAVERPROTO_CFLAGS}
    ${RESOURCEPROTO_CFLAGS}
    ${DRI2PROTO_CFLAGS}
    ${DRI3PROTO_CFLAGS}
    ${XINERAMAPROTO_CFLAGS}
    ${PIXMAN_CFLAGS}
    ${XKBFILE_CFLAGS}
    ${XFONT2_CFLAGS}
    ${XAU_CFLAGS}
    ${GL_CFLAGS}
)

if(XSHMFENCE_FOUND)
    target_compile_options(xephyr PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(xephyr PRIVATE ${LIBBSD_CFLAGS})
endif()

# Add XCB compile flags
target_compile_options(xephyr PRIVATE
    ${XCB_CFLAGS}
    ${XCB_SHAPE_CFLAGS}
    ${XCB_RENDER_CFLAGS}
    ${XCB_RENDERUTIL_CFLAGS}
    ${XCB_AUX_CFLAGS}
    ${XCB_IMAGE_CFLAGS}
    ${XCB_ICCCM_CFLAGS}
    ${XCB_SHM_CFLAGS}
    ${XCB_KEYSYMS_CFLAGS}
    ${XCB_RANDR_CFLAGS}
    ${XCB_XKB_CFLAGS}
    ${X11_XCB_CFLAGS}
    ${XCB_XV_CFLAGS}
)

if(EPOXY_FOUND)
    target_compile_options(xephyr PRIVATE ${EPOXY_CFLAGS})
endif()

# Configure dix-config.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dix-config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/dix-config.h"
)

# Configure xkb-config.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/xkb-config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/xkb-config.h"
)
