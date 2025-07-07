set(srcs_miext_damage
    damage.c
)

set(hdrs_miext_damage
    damage.h
    damagestr.h
)

add_library(libxserver_miext_damage OBJECT
    ${srcs_miext_damage}
)

target_include_directories(libxserver_miext_damage PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_miext_damage PRIVATE
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
    target_compile_options(libxserver_miext_damage PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_miext_damage PRIVATE ${LIBBSD_CFLAGS})
endif()