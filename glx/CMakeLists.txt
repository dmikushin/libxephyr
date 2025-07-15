set(srcs_glx
    indirect_dispatch.c
    indirect_dispatch_swap.c
    indirect_reqsize.c
    indirect_size_get.c
    indirect_table.c
    clientinfo.c
    createcontext.c
    extension_string.c
    indirect_util.c
    indirect_program.c
    indirect_texture_compression.c
    glxcmds.c
    glxcmdsswap.c
    glxext.c
    glxdriswrast.c
    glxdricommon.c
    glxscreens.c
    render2.c
    render2swap.c
    renderpix.c
    renderpixswap.c
    rensize.c
    single2.c
    single2swap.c
    singlepix.c
    singlepixswap.c
    singlesize.c
    swap_interval.c
    xfont.c
)

set(srcs_vnd
    vndcmds.c
    vndext.c
    vndservermapping.c
    vndservervendor.c
)

set(hdrs_vnd
    vndserver.h
)

# GLX is enabled by default for libxephyr
set(BUILD_GLX ON CACHE BOOL "Build GLX extension")
set(BUILD_DRI2 ON CACHE BOOL "Build DRI2 extension")
set(BUILD_DRI3 OFF CACHE BOOL "Build DRI3 extension")

# Find required GLX dependencies
pkg_check_modules(GLPROTO REQUIRED glproto>=1.4.17)
pkg_check_modules(GL REQUIRED gl>=1.2)

# Check for DRI dependencies
pkg_check_modules(DRI dri)

# GLX alignment for 64-bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(glx_align64 "-DGLX_ALIGN64")
else()
    set(glx_align64 "")
endif()

set(libxserver_glx "")
if(BUILD_GLX)
    add_library(libxserver_glx OBJECT
        ${srcs_glx}
    )
    
    target_include_directories(libxserver_glx PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/include
    )
    
    target_compile_options(libxserver_glx PRIVATE
        ${glx_align64}
        -Wno-unused-function
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
        ${GLPROTO_CFLAGS}
        ${GL_CFLAGS}
    )
    
    if(DRI_FOUND)
        target_compile_options(libxserver_glx PRIVATE ${DRI_CFLAGS})
    endif()
    
    if(XSHMFENCE_FOUND)
        target_compile_options(libxserver_glx PRIVATE ${XSHMFENCE_CFLAGS})
    endif()
    
    if(LIBBSD_FOUND)
        target_compile_options(libxserver_glx PRIVATE ${LIBBSD_CFLAGS})
    endif()
    
    if(DL_LIBRARY)
        target_link_libraries(libxserver_glx PRIVATE ${DL_LIBRARY})
    endif()
endif()

# GLX DRI2 sources
set(srcs_glxdri2 "")
if(BUILD_DRI2 OR BUILD_DRI3)
    set(srcs_glxdri2 glxdri2.c)
endif()

# GLX VND library
set(libglxvnd "")
if(BUILD_GLX)
    add_library(libglxvnd OBJECT
        ${srcs_vnd}
    )
    
    target_include_directories(libglxvnd PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/include
    )
    
    target_compile_options(libglxvnd PRIVATE
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
        ${GLPROTO_CFLAGS}
        ${GL_CFLAGS}
    )
    
    if(DL_LIBRARY)
        target_link_libraries(libglxvnd PRIVATE ${DL_LIBRARY})
    endif()
    
    if(XSHMFENCE_FOUND)
        target_compile_options(libglxvnd PRIVATE ${XSHMFENCE_CFLAGS})
    endif()
    
    if(LIBBSD_FOUND)
        target_compile_options(libglxvnd PRIVATE ${LIBBSD_CFLAGS})
    endif()
endif()