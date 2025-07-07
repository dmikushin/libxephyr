set(srcs_fb
    fballpriv.c
    fbarc.c
    fbbits.c
    fbblt.c
    fbbltone.c
    fbcmap_mi.c
    fbcopy.c
    fbfill.c
    fbfillrect.c
    fbfillsp.c
    fbgc.c
    fbgetsp.c
    fbglyph.c
    fbimage.c
    fbline.c
    fboverlay.c
    fbpict.c
    fbpixmap.c
    fbpoint.c
    fbpush.c
    fbscreen.c
    fbseg.c
    fbsetsp.c
    fbsolid.c
    fbtrap.c
    fbutil.c
    fbwindow.c
)

set(hdrs_fb
    fb.h
    fboverlay.h
    fbpict.h
    fbrop.h
    wfbrename.h
)

add_library(libxserver_fb OBJECT
    ${srcs_fb}
)

target_include_directories(libxserver_fb PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_fb PRIVATE
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
    target_compile_options(libxserver_fb PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_fb PRIVATE ${LIBBSD_CFLAGS})
endif()

# Set position independent code
set_target_properties(libxserver_fb PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Create wfb variant with wrapper
add_library(libxserver_wfb OBJECT
    ${srcs_fb}
)

target_include_directories(libxserver_wfb PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_wfb PRIVATE
    -DFB_ACCESS_WRAPPER
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
    target_compile_options(libxserver_wfb PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_wfb PRIVATE ${LIBBSD_CFLAGS})
endif()

# Set position independent code
set_target_properties(libxserver_wfb PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Don't build by default
set_target_properties(libxserver_wfb PROPERTIES EXCLUDE_FROM_ALL TRUE)