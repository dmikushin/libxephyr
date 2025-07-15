set(srcs_present
    present.c
    present_event.c
    present_execute.c
    present_fake.c
    present_fence.c
    present_notify.c
    present_request.c
    present_scmd.c
    present_screen.c
    present_vblank.c
)

set(hdrs_present
    present.h
    presentext.h
)

pkg_check_modules(PRESENTPROTO REQUIRED presentproto>=1.2)

add_library(libxserver_present OBJECT
    ${srcs_present}
)

target_include_directories(libxserver_present PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_present PRIVATE
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
    ${PRESENTPROTO_CFLAGS}
)

if(XSHMFENCE_FOUND)
    target_compile_options(libxserver_present PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_present PRIVATE ${LIBBSD_CFLAGS})
endif()