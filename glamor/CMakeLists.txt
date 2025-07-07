set(srcs_glamor
    glamor.c
    glamor_copy.c
    glamor_core.c
    glamor_dash.c
    glamor_font.c
    glamor_glx.c
    glamor_composite_glyphs.c
    glamor_image.c
    glamor_lines.c
    glamor_segs.c
    glamor_render.c
    glamor_gradient.c
    glamor_prepare.c
    glamor_program.c
    glamor_rects.c
    glamor_spans.c
    glamor_text.c
    glamor_transfer.c
    glamor_transform.c
    glamor_trapezoid.c
    glamor_triangles.c
    glamor_addtraps.c
    glamor_glyphblt.c
    glamor_points.c
    glamor_pixmap.c
    glamor_largepixmap.c
    glamor_picture.c
    glamor_vbo.c
    glamor_window.c
    glamor_fbo.c
    glamor_compositerects.c
    glamor_utils.c
    glamor_sync.c
)

# Add XV support
list(APPEND srcs_glamor glamor_xv.c)

# Find epoxy dependency (required for glamor)
pkg_check_modules(EPOXY REQUIRED epoxy)

add_library(glamor OBJECT
    ${srcs_glamor}
)

target_include_directories(glamor PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(glamor PRIVATE
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
    ${EPOXY_CFLAGS}
)

if(XSHMFENCE_FOUND)
    target_compile_options(glamor PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(glamor PRIVATE ${LIBBSD_CFLAGS})
endif()

# Create glamor EGL stubs
add_library(glamor_egl_stubs OBJECT
    glamor_egl_stubs.c
)

target_include_directories(glamor_egl_stubs PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

target_compile_options(glamor_egl_stubs PRIVATE
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
    target_compile_options(glamor_egl_stubs PRIVATE ${XSHMFENCE_CFLAGS})
endif()

if(LIBBSD_FOUND)
    target_compile_options(glamor_egl_stubs PRIVATE ${LIBBSD_CFLAGS})
endif()

# Define libxserver_glamor for use in main build
set(libxserver_glamor glamor PARENT_SCOPE)

# Create an alias for compatibility
add_library(libxserver_glamor ALIAS glamor)