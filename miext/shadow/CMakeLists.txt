set(srcs_miext_shadow
    shadow.c
    sh3224.c
    shafb4.c
    shafb8.c
    shiplan2p4.c
    shiplan2p8.c
    shpacked.c
    shplanar8.c
    shplanar.c
    shrot16pack_180.c
    shrot16pack_270.c
    shrot16pack_270YX.c
    shrot16pack_90.c
    shrot16pack_90YX.c
    shrot16pack.c
    shrot32pack_180.c
    shrot32pack_270.c
    shrot32pack_90.c
    shrot32pack.c
    shrot8pack_180.c
    shrot8pack_270.c
    shrot8pack_90.c
    shrot8pack.c
    shrotate.c
)

set(hdrs_miext_shadow
    shadow.h
)

add_library(libxserver_miext_shadow OBJECT
    ${srcs_miext_shadow}
)

target_include_directories(libxserver_miext_shadow PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_miext_shadow PRIVATE
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
    target_compile_options(libxserver_miext_shadow PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_miext_shadow PRIVATE ${LIBBSD_CFLAGS})
endif()