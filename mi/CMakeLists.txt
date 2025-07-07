set(srcs_mi
    miarc.c
    mibitblt.c
    micmap.c
    micopy.c
    midash.c
    midispcur.c
    mieq.c
    miexpose.c
    mifillarc.c
    mifillrct.c
    migc.c
    miglblt.c
    mioverlay.c
    mipointer.c
    mipoly.c
    mipolypnt.c
    mipolyrect.c
    mipolyseg.c
    mipolytext.c
    mipushpxl.c
    miscrinit.c
    misprite.c
    mivaltree.c
    miwideline.c
    miwindow.c
    mizerarc.c
    mizerclip.c
    mizerline.c
)

set(hdrs_mi
    micmap.h
    micoord.h
    migc.h
    mi.h
    miline.h
    mioverlay.h
    mipointer.h
    mipointrst.h
    mistruct.h
    mizerarc.h
)

add_library(libxserver_mi OBJECT
    ${srcs_mi}
)

target_include_directories(libxserver_mi PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_mi PRIVATE
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
    target_compile_options(libxserver_mi PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_mi PRIVATE ${LIBBSD_CFLAGS})
endif()

# Link with math library
if(M_LIBRARY)
    target_link_libraries(libxserver_mi PRIVATE ${M_LIBRARY})
endif()