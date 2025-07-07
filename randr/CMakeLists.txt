set(srcs_randr
    randr.c
    rrcrtc.c
    rrdispatch.c
    rrinfo.c
    rrlease.c
    rrmode.c
    rrmonitor.c
    rroutput.c
    rrpointer.c
    rrproperty.c
    rrprovider.c
    rrproviderproperty.c
    rrscreen.c
    rrsdispatch.c
    rrtransform.c
)

set(hdrs_randr
    randrstr.h
    rrtransform.h
)

# Add Xinerama support
list(APPEND srcs_randr rrxinerama.c)

add_library(libxserver_randr OBJECT
    ${srcs_randr}
)

target_include_directories(libxserver_randr PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_randr PRIVATE
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
    target_compile_options(libxserver_randr PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_randr PRIVATE ${LIBBSD_CFLAGS})
endif()