# Main Xephyr sources
set(srcs
    ephyr.c
    ephyrinit.c
    ephyrcursor.c
    ephyr_draw.c
    hostx.c
)

# Library-specific sources
set(libxephyr_srcs
    ephyr.c
    ephyr_lib.c
    ephyrcursor.c
    ephyr_draw.c
    hostx.c
    libxephyr_api.c
)

# XCB dependencies are defined in root CMakeLists.txt

# Add Glamor support
list(APPEND srcs ephyr_glamor_glx.c ephyr_glamor_xv.c ephyrvideo.c)
list(APPEND libxephyr_srcs ephyr_glamor_glx.c ephyr_glamor_xv.c ephyrvideo.c)

# Create libxephyr object library  
add_library(ephyr_lib OBJECT
    ${libxephyr_srcs}
)

target_include_directories(ephyr_lib PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
    ../src
    ../../../exa
)

target_compile_options(ephyr_lib PRIVATE
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
    ${XCB_CFLAGS}
    ${XCB_SHAPE_CFLAGS}
    ${XCB_RENDER_CFLAGS}
    ${XCB_RENDERUTIL_CFLAGS}
    ${XCB_AUX_CFLAGS}
    ${XCB_IMAGE_CFLAGS}
    ${XCB_ICCCM_CFLAGS}
    ${XCB_SHM_CFLAGS}
    ${XCB_KEYSYMS_CFLAGS}
    ${XCB_RANDR_CFLAGS}
    ${XCB_XKB_CFLAGS}
    ${X11_XCB_CFLAGS}
    ${XCB_XV_CFLAGS}
)

if(EPOXY_FOUND)
    target_compile_options(ephyr_lib PRIVATE ${EPOXY_CFLAGS})
endif()

if(XSHMFENCE_FOUND)
    target_compile_options(ephyr_lib PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(ephyr_lib PRIVATE ${LIBBSD_CFLAGS})
endif()

# OBJECT libraries don't link, dependencies will be handled by the main library
# Install header
install(FILES libxephyr.h DESTINATION include/libxephyr)

# Add the hardware component to the main build
set(libxephyr_hw libxephyr_hw PARENT_SCOPE)