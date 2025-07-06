srcs_xext = [
    'bigreq.c',
    'geext.c',
    'shape.c',
    'sleepuntil.c',
    'sync.c',
    'xcmisc.c',
    'xtest.c',
]

hdrs_xext = [
    'geext.h',
    'geint.h',
    'syncsdk.h',
]

srcs_xext += 'dpms.c'
srcs_xext += 'shm.c'
hdrs_xext += ['shmint.h']
srcs_xext += 'hashtable.c'
srcs_xext += 'xres.c'
srcs_xext += 'saver.c'
srcs_xext += 'xace.c'
hdrs_xext += ['xace.h', 'xacestr.h']
#srcs_xext += 'xf86bigfont.c'
srcs_xext += ['panoramiX.c', 'panoramiXprocs.c', 'panoramiXSwap.c']
hdrs_xext += ['panoramiX.h', 'panoramiXsrv.h']
#srcs_xext += ['security.c']
#srcs_xext += ['xselinux_ext.c', 'xselinux_hooks.c', 'xselinux_label.c']
srcs_xext += ['xvmain.c', 'xvdisp.c', 'xvmc.c']
hdrs_xext += ['xvdix.h', 'xvmcext.h']

libxserver_xext = static_library('libxserver_xext',
    srcs_xext,
    include_directories: inc,
    dependencies: common_dep,
)
