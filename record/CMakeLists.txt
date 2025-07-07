set(srcs_record
    record.c
    set.c
)

add_library(libxserver_record OBJECT
    ${srcs_record}
)

target_include_directories(libxserver_record PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_record PRIVATE
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
    target_compile_options(libxserver_record PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_record PRIVATE ${LIBBSD_CFLAGS})
endif()