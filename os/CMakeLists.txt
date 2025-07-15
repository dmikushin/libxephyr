set(srcs_os
    WaitFor.c
    access.c
    auth.c
    backtrace.c
    client.c
    connection.c
    inputthread.c
    io.c
    mitauth.c
    oscolor.c
    osinit.c
    ospoll.c
    utils.c
    xdmauth.c
    xsha1.c
    xstrans.c
    xprintf.c
    log.c
)

# Wrapper code for missing C library functions
set(srcs_libc "")
if(NOT HAVE_REALLOCARRAY)
    list(APPEND srcs_libc reallocarray.c)
endif()
if(NOT HAVE_STRCASECMP)
    list(APPEND srcs_libc strcasecmp.c)
endif()
if(NOT HAVE_STRCASESTR)
    list(APPEND srcs_libc strcasestr.c)
endif()
if(NOT HAVE_STRLCAT)
    list(APPEND srcs_libc strlcat.c)
endif()
if(NOT HAVE_STRLCPY)
    list(APPEND srcs_libc strlcpy.c)
endif()
if(NOT HAVE_STRNDUP)
    list(APPEND srcs_libc strndup.c)
endif()
if(NOT HAVE_TIMINGSAFE_MEMCMP)
    list(APPEND srcs_libc timingsafe_memcmp.c)
endif()
if(NOT HAVE_POLL)
    list(APPEND srcs_os xserver_poll.c)
endif()

if(BUSFAULT)
    list(APPEND srcs_os busfault.c)
endif()

# XDMCP support
set(XDMCP OFF CACHE BOOL "Enable XDMCP support")
if(XDMCP)
    list(APPEND srcs_os xdmcp.c)
endif()

# Secure RPC support
set(SECURE_RPC OFF CACHE BOOL "Enable secure RPC")
if(SECURE_RPC)
    pkg_check_modules(TIRPC libtirpc)
    if(NOT TIRPC_FOUND)
        check_include_file(rpc/rpc.h HAVE_RPC_H)
        if(NOT HAVE_RPC_H)
            message(FATAL_ERROR "secure-rpc requested, but neither libtirpc or libc RPC support were found")
        endif()
    endif()
    list(APPEND srcs_os rpcauth.c)
endif()

# Add XRes extension client ID support
set(XRES OFF CACHE BOOL "Enable XRes extension")
set(os_c_args "")
if(XRES)
    list(APPEND os_c_args "-DCLIENTIDS")
endif()

add_library(libxserver_os OBJECT
    ${srcs_os} ${srcs_libc}
)

target_include_directories(libxserver_os PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_os PRIVATE
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
    # Include path for our custom libXfont2
    -I${CMAKE_SOURCE_DIR}/ThirdParty/libxfont/include
    ${os_c_args}
)

if(XSHMFENCE_FOUND)
    target_compile_options(libxserver_os PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_os PRIVATE ${LIBBSD_CFLAGS})
endif()

# Add XAU dependency
pkg_check_modules(XAU REQUIRED xau)
target_compile_options(libxserver_os PRIVATE ${XAU_CFLAGS})

# Add pthread for input thread
if(INPUTTHREAD)
    find_package(Threads REQUIRED)
    target_link_libraries(libxserver_os PRIVATE Threads::Threads)
endif()

# Add RPC dependency if needed
if(SECURE_RPC AND TIRPC_FOUND)
    target_compile_options(libxserver_os PRIVATE ${TIRPC_CFLAGS})
endif()
