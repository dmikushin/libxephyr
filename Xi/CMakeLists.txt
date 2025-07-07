set(srcs_xi
    allowev.c
    chgdctl.c
    chgfctl.c
    chgkbd.c
    chgkmap.c
    chgprop.c
    chgptr.c
    closedev.c
    devbell.c
    exevents.c
    extinit.c
    getbmap.c
    getdctl.c
    getfctl.c
    getfocus.c
    getkmap.c
    getmmap.c
    getprop.c
    getselev.c
    getvers.c
    grabdev.c
    grabdevb.c
    grabdevk.c
    gtmotion.c
    listdev.c
    opendev.c
    queryst.c
    selectev.c
    sendexev.c
    setbmap.c
    setdval.c
    setfocus.c
    setmmap.c
    setmode.c
    ungrdev.c
    ungrdevb.c
    ungrdevk.c
    xiallowev.c
    xibarriers.c
    xichangecursor.c
    xichangehierarchy.c
    xigetclientpointer.c
    xigrabdev.c
    xipassivegrab.c
    xiproperty.c
    xiquerydevice.c
    xiquerypointer.c
    xiqueryversion.c
    xiselectev.c
    xisetclientpointer.c
    xisetdevfocus.c
    xiwarppointer.c
)

add_library(libxserver_xi OBJECT
    ${srcs_xi}
)

target_include_directories(libxserver_xi PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_xi PRIVATE
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
    target_compile_options(libxserver_xi PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_xi PRIVATE ${LIBBSD_CFLAGS})
endif()

# XI stubs
set(srcs_xi_stubs stubs.c)

add_library(libxserver_xi_stubs OBJECT
    ${srcs_xi_stubs}
)

target_include_directories(libxserver_xi_stubs PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_xi_stubs PRIVATE
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
    target_compile_options(libxserver_xi_stubs PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_xi_stubs PRIVATE ${LIBBSD_CFLAGS})
endif()