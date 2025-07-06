project('libxephyr', 'c',
        default_options: [
            'buildtype=debugoptimized',
            'c_std=gnu99',
        ],
        version: '21.1.4',
        meson_version: '>= 0.47.0',
)

add_project_arguments('-DHAVE_DIX_CONFIG_H', '-D_XSERVER64', language: ['c', 'objc'])
cc = meson.get_compiler('c')

# Basic dependencies needed for libxephyr
xproto_dep = dependency('xproto', version: '>= 7.0.31')
randrproto_dep = dependency('randrproto', version: '>= 1.6.0')
renderproto_dep = dependency('renderproto', version: '>= 0.11')
xextproto_dep = dependency('xextproto', version: '>= 7.2.99.901')
inputproto_dep = dependency('inputproto', version: '>= 2.3.99.1')
kbproto_dep = dependency('kbproto', version: '>= 1.0.3')
fontsproto_dep = dependency('fontsproto', version: '>= 2.1.3')
fixesproto_dep = dependency('fixesproto', version: '>= 6.0')
damageproto_dep = dependency('damageproto', version: '>= 1.1')
xcmiscproto_dep = dependency('xcmiscproto', version: '>= 1.2.0')
bigreqsproto_dep = dependency('bigreqsproto', version: '>= 1.1.0')
xtrans_dep = dependency('xtrans', version: '>= 1.3.5')
videoproto_dep = dependency('videoproto')
compositeproto_dep = dependency('compositeproto', version: '>= 0.4')
recordproto_dep = dependency('recordproto', version: '>= 1.13.99.1')
scrnsaverproto_dep = dependency('scrnsaverproto', version: '>= 1.1')
resourceproto_dep = dependency('resourceproto', version: '>= 1.2.0')
dri2proto_dep = dependency('dri2proto', version: '>= 2.8')
dri3proto_dep = dependency('dri3proto', version: '>= 1.2')
xineramaproto_dep = dependency('xineramaproto')
xshmfence_dep = dependency('xshmfence', version: '>= 1.1', required: false)
pixman_dep = dependency('pixman-1')
libbsd_dep = dependency('libbsd', required: false)
xkbfile_dep = dependency('xkbfile')
xfont2_dep = dependency('xfont2', version: '>= 2.0')
dbus_dep = dependency('dbus-1', version: '>= 1.0', required: false)
libdrm_dep = dependency('libdrm', required: false)
epoxy_dep = dependency('epoxy', required: false)
gbm_dep = dependency('gbm', required: false)
m_dep = cc.find_library('m', required : false)
dl_dep = cc.find_library('dl', required : false)
hal_dep = dependency('hal', required: false)
udev_dep = dependency('libudev', required: false)

# XKB configuration
xkb_dir = get_option('xkb_dir')
if xkb_dir == ''
    xkb_dir = join_paths(get_option('prefix'), 'share/X11/xkb')
endif

xkb_output_dir = get_option('xkb_output_dir')
if xkb_output_dir == ''
    xkb_output_dir = join_paths(xkb_dir, 'compiled')
endif

xkb_bin_dir = get_option('xkb_bin_dir')
if xkb_bin_dir == ''
    xkb_bin_dir = join_paths(get_option('prefix'), get_option('bindir'))
endif

common_dep = [
    xproto_dep,
    randrproto_dep,
    renderproto_dep,
    xextproto_dep,
    inputproto_dep,
    kbproto_dep,
    fontsproto_dep,
    fixesproto_dep,
    damageproto_dep,
    xcmiscproto_dep,
    bigreqsproto_dep,
    xtrans_dep,
    videoproto_dep,
    compositeproto_dep,
    recordproto_dep,
    scrnsaverproto_dep,
    resourceproto_dep,
    dri2proto_dep,
    dri3proto_dep,
    xineramaproto_dep,
    xshmfence_dep,
    pixman_dep,
    libbsd_dep,
    xkbfile_dep,
    xfont2_dep,
    m_dep,
    dl_dep,
]

inc = include_directories(
    '.',
    'Xext',
    'Xi',
    'composite',
    'damageext',
    'fb',
    'glamor',
    'mi',
    'miext/damage',
    'miext/shadow',
    'miext/sync',
    'dbe',
    'include',
    'present',
    'randr',
    'render',
    'xfixes',
)

# Include must come first, as it sets up dix-config.h
subdir('include')

# Core X server components needed for Xephyr
subdir('dix')
subdir('fb')
subdir('mi')
subdir('os')

# Essential extensions for Xephyr
subdir('miext/damage')
subdir('miext/shadow')
subdir('miext/sync')
subdir('randr')
subdir('render')
subdir('xfixes')
subdir('xkb')
subdir('Xext')
subdir('Xi')
subdir('glamor')
subdir('exa')
subdir('present')
subdir('composite')
subdir('damageext')
subdir('dbe')
subdir('record')
subdir('config')
subdir('hw')

libxephyr = static_library('xephyr',
    sources: [
        libxserver_mi,
        libxserver_dix,
        libxserver_fb,
        libxserver_os,
        libxserver_miext_damage,
        libxserver_miext_shadow,
        libxserver_miext_sync,
        libxserver_randr,
        libxserver_render,
        libxserver_xfixes,
        libxserver_xkb,
        libxserver_xext,
        libxserver_xi,
        libxserver_composite,
        libxserver_damageext,
        libxserver_dbe,
        libxserver_present,
        libxserver_record,
        libxserver_config,
        libxserver_exa,
        libxserver_glamor,
        libxephyr_hw,
    ],
    dependencies: common_dep,
    include_directories: inc,
    c_args: ['-fvisibility=hidden']
)