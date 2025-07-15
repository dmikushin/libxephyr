set(srcs_exa
    exa.c
    exa_classic.c
    exa_migration_classic.c
    exa_driver.c
    exa_mixed.c
    exa_migration_mixed.c
    exa_accel.c
    exa_glyphs.c
    exa_offscreen.c
    exa_render.c
    exa_unaccel.c
)

add_library(libxserver_exa OBJECT
    ${srcs_exa}
)

target_include_directories(libxserver_exa PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(libxserver_exa PRIVATE
    -DHAVE_XORG_CONFIG_H
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
    target_compile_options(libxserver_exa PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(libxserver_exa PRIVATE ${LIBBSD_CFLAGS})
endif()