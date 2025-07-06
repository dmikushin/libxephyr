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

xephyr_glamor = [glamor, glamor_egl_stubs]
srcs += ['ephyr_glamor_glx.c', 'ephyr_glamor_xv.c', 'ephyrvideo.c']
xephyr_dep += [dependency('x11-xcb'), dependency('xcb-xv'), epoxy_dep]

# Create libxephyr static library
libxephyr_srcs = [
    'ephyr.c',
    'ephyr_lib.c',
    'ephyrcursor.c',
    'ephyr_draw.c',
    'hostx.c',
    'libxephyr_api.c',
]

libxephyr_glamor = [glamor, glamor_egl_stubs]
libxephyr_srcs += ['ephyr_glamor_glx.c', 'ephyr_glamor_xv.c', 'ephyrvideo.c']

libxephyr = static_library(
    'xephyr',
    libxephyr_srcs,
    include_directories: [
        inc,
        include_directories('../src'),
        include_directories('../../../exa')
    ],
    dependencies: xephyr_dep,
    install: true,
)

# Install the header file
install_headers('libxephyr.h', subdir: 'libxephyr')

# Export libxephyr dependency for subprojects
libxephyr_dep = declare_dependency(
    link_with: [
        libxephyr,
        libxserver_exa,
        xephyr_glamor,
        kdrive,
        libxserver_fb,
        libxserver_config,
    ],
    include_directories: include_directories('.'),
    dependencies: xephyr_dep
)