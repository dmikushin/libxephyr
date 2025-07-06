conf_data = configuration_data()

# Always enable GLX and Glamor for libxephyr
conf_data.set('GLXEXT', '1')
conf_data.set('GLAMOR', '1')

# Basic configuration for dix
conf_data.set('_DIX_CONFIG_H_', '1')
if host_machine.endian() == 'little'
    conf_data.set('X_BYTE_ORDER', 'X_LITTLE_ENDIAN')
else
    conf_data.set('X_BYTE_ORDER', 'X_BIG_ENDIAN')
endif
# For libxephyr, we need _XSERVER64 to make XID = CARD32 (4 bytes)
if cc.sizeof('unsigned long') == 8
   conf_data.set('_XSERVER64', '1')
endif
conf_data.set('_GNU_SOURCE', '1')

# Hardcode dependencies that we know we need
conf_data.set('HAVE_XSHMFENCE', xshmfence_dep.found() ? '1' : false)
conf_data.set('WITH_LIBDRM', libdrm_dep.found() ? '1' : false)
conf_data.set('GLAMOR_HAS_GBM', gbm_dep.found() ? '1' : false)
conf_data.set('HAVE_REALLOCARRAY', cc.has_function('reallocarray', dependencies: libbsd_dep) ? '1' : false)
conf_data.set('HAVE_STRCASECMP', cc.has_function('strcasecmp') ? '1' : false)
conf_data.set('HAVE_STRCASESTR', cc.has_function('strcasestr') ? '1' : false)
conf_data.set('HAVE_STRLCAT', cc.has_function('strlcat', dependencies: libbsd_dep) ? '1' : false)
conf_data.set('HAVE_STRLCPY', cc.has_function('strlcpy', dependencies: libbsd_dep) ? '1' : false)
conf_data.set('HAVE_STRNDUP', cc.has_function('strndup') and cc.has_header_symbol('string.h', 'strndup') ? '1' : false)
conf_data.set('HAVE_TIMINGSAFE_MEMCMP', cc.has_function('timingsafe_memcmp') ? '1' : false)
conf_data.set('HAVE_POLL', cc.has_function('poll') ? '1' : false)
conf_data.set('BUSFAULT', cc.has_function('sigaction') ? '1' : false)
enable_input_thread = cc.has_header_symbol('pthread.h', 'PTHREAD_MUTEX_RECURSIVE')
conf_data.set('INPUTTHREAD', enable_input_thread ? '1' : false)

# Check for peer credential functions
if cc.has_function('getpeereid', dependencies: libbsd_dep)
    conf_data.set('HAVE_GETPEEREID', 1)
endif
if cc.has_function('getpeerucred')
    conf_data.set('HAVE_GETPEERUCRED', 1)
endif

# Check for local client credentials
have_getpeereid = cc.has_function('getpeereid')
have_getpeerucred = cc.has_function('getpeerucred')
if not have_getpeereid and not have_getpeerucred
    if not cc.has_header_symbol('sys/socket.h', 'SO_PEERCRED')
        conf_data.set('NO_LOCAL_CLIENT_CRED', 1)
    endif
endif

# Set default font path - standard X11 font directories
conf_data.set_quoted('COMPILEDDEFAULTFONTPATH', '/usr/share/fonts/X11/misc/,/usr/share/fonts/X11/TTF/,/usr/share/fonts/X11/OTF/,/usr/share/fonts/X11/Type1/,/usr/share/fonts/X11/100dpi/,/usr/share/fonts/X11/75dpi/')

# Hardcode some feature flags
conf_data.set('COMPOSITE', '1')
conf_data.set('DAMAGE', '1')
conf_data.set('DBE', '1')
conf_data.set('DRI2', '1')
conf_data.set('DRI3', '1')
conf_data.set('HAS_SHM', '1')
conf_data.set('MITSHM', '1')
conf_data.set('PANORAMIX', '1')
conf_data.set('PRESENT', '1')
conf_data.set('RANDR', '1')
conf_data.set('RENDER', '1')
conf_data.set('SCREENSAVER', '1')
conf_data.set('SHAPE', '1')
conf_data.set('XACE', '1')
conf_data.set('XFIXES', '1')
conf_data.set('XINERAMA', '1')
conf_data.set('XINPUT', '1')
conf_data.set('XRECORD', '1')
conf_data.set('XSYNC', '1')
conf_data.set('XTEST', '1')
conf_data.set('XV', '1')
conf_data.set('XvExtension', '1')

configure_file(output : 'dix-config.h',
               configuration : conf_data)

# XKB configuration
xkb_conf_data = configuration_data()
xkb_conf_data.set_quoted('XKB_BASE_DIRECTORY', xkb_dir)
xkb_conf_data.set_quoted('XKB_BIN_DIRECTORY', xkb_bin_dir)
xkb_conf_data.set_quoted('XKB_COMPILED_DIR', xkb_output_dir)
xkb_conf_data.set_quoted('XKM_OUTPUT_DIR', xkb_output_dir)
xkb_conf_data.set_quoted('XKB_DFLT_RULES_FILE', 'base')
xkb_conf_data.set_quoted('XKB_DFLT_KB_LAYOUT', 'us')
xkb_conf_data.set_quoted('XKB_DFLT_KB_MODEL', 'pc105')
xkb_conf_data.set_quoted('XKB_DFLT_KB_VARIANT', '')
xkb_conf_data.set_quoted('XKB_DFLT_KB_OPTIONS', '')

configure_file(output : 'xkb-config.h',
               configuration : xkb_conf_data)