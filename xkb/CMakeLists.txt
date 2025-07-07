set(srcs_xkb
    ddxBeep.c
    ddxCtrls.c
    ddxLEDs.c
    ddxLoad.c
    maprules.c
    xkmread.c
    xkbtext.c
    xkbfmisc.c
    xkbout.c
    xkb.c
    xkbUtils.c
    xkbEvents.c
    xkbAccessX.c
    xkbSwap.c
    xkbLEDs.c
    xkbInit.c
    xkbActions.c
    xkbPrKeyEv.c
    XKBMisc.c
    XKBAlloc.c
    XKBGAlloc.c
    XKBMAlloc.c
)

add_library(libxserver_xkb OBJECT
    ${srcs_xkb}
)

target_include_directories(libxserver_xkb PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_xkb PRIVATE
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
)

if(XSHMFENCE_FOUND)
    target_compile_options(libxserver_xkb PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_xkb PRIVATE ${LIBBSD_CFLAGS})
endif()

# XKB stubs
set(srcs_xkb_stubs
    ddxKillSrv.c
    ddxPrivate.c
    ddxVT.c
)

add_library(libxserver_xkb_stubs OBJECT
    ${srcs_xkb_stubs}
)

target_include_directories(libxserver_xkb_stubs PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_xkb_stubs PRIVATE
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
)

if(XSHMFENCE_FOUND)
    target_compile_options(libxserver_xkb_stubs PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_xkb_stubs PRIVATE ${LIBBSD_CFLAGS})
endif()