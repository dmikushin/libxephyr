srcs_config = [
    'config.c',
]

config_dep = [common_dep, udev_dep]
srcs_config += ['udev.c']

libxserver_config = static_library('libxserver_config',
    srcs_config,
    include_directories: inc,
    dependencies: config_dep,
)
