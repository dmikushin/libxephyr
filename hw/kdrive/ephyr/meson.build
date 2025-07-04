srcs = [
    'ephyr.c',
    'ephyrinit.c',
    'ephyrcursor.c',
    'ephyr_draw.c',
    'hostx.c',
]

xephyr_dep = [
    common_dep,
    dependency('xcb'),
    dependency('xcb-shape'),
    dependency('xcb-render'),
    dependency('xcb-renderutil'),
    dependency('xcb-aux'),
    dependency('xcb-image'),
    dependency('xcb-icccm'),
    dependency('xcb-shm', version : '>=1.9.3'),
    dependency('xcb-keysyms'),
    dependency('xcb-randr'),
    dependency('xcb-xkb'),
]

xephyr_glamor = []
if build_glamor
    srcs += 'ephyr_glamor_glx.c'
    if build_xv
        srcs += 'ephyr_glamor_xv.c'
    endif
    xephyr_glamor += glamor
    xephyr_glamor += glamor_egl_stubs
    xephyr_dep += dependency('x11-xcb')
    xephyr_dep += epoxy_dep
endif

if build_xv
    srcs += 'ephyrvideo.c'
    xephyr_dep += dependency('xcb-xv')
endif

xephyr_server = executable(
    'Xephyr',
    srcs,
    include_directories: [
        inc,
        include_directories('../src')
    ],
    dependencies: xephyr_dep,
    link_with: [
        libxserver_main,
        libxserver_exa,
        xephyr_glamor,
        kdrive,
        libxserver_fb,
        libxserver,
        libxserver_config,
        libxserver_xkb_stubs,
        libxserver_xi_stubs,
        libxserver_glx,
        libglxvnd,
    ],
    install: true,
)

xephyr_man = configure_file(
    input: 'man/Xephyr.man',
    output: 'Xephyr.1',
    configuration: manpage_config,
)
install_man(xephyr_man)

# Create libxephyr shared library
libxephyr_srcs = [
    'ephyr.c',
    'ephyr_lib.c',
    'ephyrcursor.c',
    'ephyr_draw.c',
    'hostx.c',
    'libxephyr_api.c',
]

libxephyr_glamor = []
if build_glamor
    libxephyr_srcs += 'ephyr_glamor_glx.c'
    if build_xv
        libxephyr_srcs += 'ephyr_glamor_xv.c'
    endif
    libxephyr_glamor += glamor
    libxephyr_glamor += glamor_egl_stubs
endif

if build_xv
    libxephyr_srcs += 'ephyrvideo.c'
endif

libxephyr = shared_library(
    'xephyr',
    libxephyr_srcs,
    include_directories: [
        inc,
        include_directories('../src')
    ],
    dependencies: xephyr_dep,
    link_with: [
        libxserver_main,
        libxserver_exa,
        libxephyr_glamor,
        kdrive,
        libxserver_fb,
        libxserver,
        libxserver_config,
        libxserver_xkb_stubs,
        libxserver_xi_stubs,
        libxserver_glx,
        libglxvnd,
    ],
    install: true,
    version: '21.1.4',
    soversion: '21',
)

# Install the header file
install_headers('libxephyr.h', subdir: 'libxephyr')

# Export libxephyr dependency for subprojects
libxephyr_dep = declare_dependency(
    link_with: libxephyr,
    include_directories: include_directories('.'),
    dependencies: xephyr_dep
)
