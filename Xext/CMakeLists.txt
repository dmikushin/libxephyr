set(srcs_xext
    bigreq.c
    geext.c
    shape.c
    sleepuntil.c
    sync.c
    xcmisc.c
    xtest.c
)

set(hdrs_xext
    geext.h
    geint.h
    syncsdk.h
)

# Add extensions
list(APPEND srcs_xext dpms.c)
list(APPEND srcs_xext shm.c)
list(APPEND hdrs_xext shmint.h)
list(APPEND srcs_xext hashtable.c)
list(APPEND srcs_xext xres.c)
list(APPEND srcs_xext saver.c)
list(APPEND srcs_xext xace.c)
list(APPEND hdrs_xext xace.h xacestr.h)
list(APPEND srcs_xext panoramiX.c panoramiXprocs.c panoramiXSwap.c)
list(APPEND hdrs_xext panoramiX.h panoramiXsrv.h)
list(APPEND srcs_xext xvmain.c xvdisp.c xvmc.c)
list(APPEND hdrs_xext xvdix.h xvmcext.h)

add_library(libxserver_xext OBJECT
    ${srcs_xext}
)

target_include_directories(libxserver_xext PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_xext PRIVATE
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
)

if(XSHMFENCE_FOUND)
    target_compile_options(libxserver_xext PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_xext PRIVATE ${LIBBSD_CFLAGS})
endif()