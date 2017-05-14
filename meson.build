# Meson build file

# https://github.com/linuxmint/nemo
project('nemo', 'c', version : '3.4.1')

# 1. If the library code has changed at all since last release, then increment revision.
# 2. If any interfaces have been added, then increment current and set revision to 0.
# Interface break is not allowed.
nemo_extension_current  = 5
nemo_extension_revision = 0
# We need to decrement current by one in the calculation of the age because
# the library was started with version "1:0:0" instead of "0:0:0"
NEMO_EXTENSION_VERSION_INFO = '@0@:@1@:@2@'.format(
  nemo_extension_current, nemo_extension_revision, nemo_extension_current - 1)

compiler = meson.get_compiler('c')
prefix = get_option('prefix')
buildtype = get_option('buildtype')

################################################################################

conf = configuration_data()
# Surround the version in quotes to make it a C string
conf.set_quoted('VERSION', meson.project_version())
conf.set('ENABLE_DEBUG', buildtype == 'debug')

check_headers = [
  'malloc.h',
  'sys/mount.h',
  'sys/param.h',
  'sys/vfs.h',
  'X11/XF86keysym.h'
]

foreach h : check_headers
  conf.set10('HAVE_' + h.underscorify().to_upper(), compiler.has_header(h))
endforeach

conf.set10('HAVE_MALLOPT', compiler.has_function('mallopt', prefix : '#include <malloc.h>'))

################################################################################
# Translation
i18n = import('i18n')

i18n.gettext('nemo',
    languages : [])

conf.set_quoted('GETTEXT_PACKAGE', 'nemo')
conf.set('ENABLE_NLS', true)
conf.set('HAVE_GETTEXT', true)
conf.set('HAVE_LOCALE_H', true)
conf.set_quoted('LOCALE_DIR', '/usr/share/locale')
conf.set_quoted('LOCALEDIR',  '/usr/share/locale')


################################################################################
add_global_arguments('-Wno-deprecated-declarations', language : 'c')

## GTK_DOC_CHECK([1.4])

################################################################################
# Arguments and dependencies checking

# libexif
if get_option('exif')
  libexif = dependency('libexif', version : '>=0.6.20')
  conf.set('HAVE_EXIF', true)
  libexif_enabled = true
else
  conf.set('HAVE_EXIF', false)
  libexif_enabled = false
endif

# exempi checking
if get_option('xmp')
  exempi = dependency('exempi-2.0', version : '>=2.2.0')
  conf.set('HAVE_EXEMPI', true)
  exempi_enabled = true
else
  conf.set('HAVE_EXEMPI', false)
  exempi_enabled = false
endif

conf.set('GENERIC_ICON_API', true)

# libselinux
if get_option('selinux')
  libselinux = dependency('libselinux', version : '>=2.0')
  conf.set('HAVE_SELINUX', true)
  libselinux_enabled = true
else
  conf.set('HAVE_SELINUX', false)
  libselinux_enabled = false
endif

# Debug Empty View
if get_option('empty-view')
  conf.set10('ENABLE_EMPTY_VIEW', true)
  enableEmptyView = true
else
  conf.set10('ENABLE_EMPTY_VIEW', false)
  enableEmptyView = false
endif

# Check for Tracker
trackerChoice = get_option('enable-tracker')

if trackerChoice == 'false'
  conf.set('ENABLE_TRACKER', false)
  tracker_enabled = false
else
    tracker_sparql = dependency('tracker-sparql-1.0',  required : false)
  if not tracker_sparql.found()
    tracker_sparql = dependency('tracker-sparql-0.18', required : false)
  endif
  if not tracker_sparql.found()
    tracker_sparql = dependency('tracker-sparql-0.16', required : (trackerChoice == 'true'))
  endif

  tracker_enabled = (trackerChoice == 'true') or tracker_sparql.found()
  conf.set('ENABLE_TRACKER', tracker_enabled ? true : false)
endif


enable_update_mimedb = get_option('update-mimedb')

updateMimeDB = find_program('update-mime-database',
    required : enable_update_mimedb)


################################################################################
# Find Dependencies

perl        = find_program('perl5', 'perl')
genMarshal  = find_program('glib-genmarshal')

math    = compiler.find_library('m', required : true)

glib    = dependency('glib-2.0', version : '>=2.37.3')
gtk     = dependency('gtk+-3.0', version : '>=3.9.10')

cinnamon= dependency('cinnamon-desktop', version : '>=2.6.1')
gail    = dependency('gail-3.0')
libxml  = dependency('libxml-2.0', version : '>=2.7.8')
x11     = dependency('x11')

gthread = dependency('gthread-2.0', version : '>= 2.37.3')
gmodule = dependency('gmodule-no-export-2.0', version : '>= 2.37.3')
gio     = dependency('gio-2.0', version : '>= 2.37.3')
gobject = dependency('gobject-2.0', version : '>=2.0')
go_intr = dependency('gobject-introspection-1.0', version : '>=1.0')
gio_unix= dependency('gio-unix-2.0', version : '>= 2.37.3')
libnotify=dependency('libnotify', version : '>= 0.7.0')


gnome = import('gnome')

################################################################################

rootInclude = include_directories('.')

subdir('cut-n-paste-code/libegg')
subdir('data')
subdir('eel')
subdir('files')
subdir('libnemo-extension')
subdir('libnemo-private')
# subdir('po/Makefile.in')
subdir('src')
subdir('test')

gtkdoc_enabled = false
if build_machine.system() == 'windows'
  message('Disabling gtk-doc while building on Windows')
else
  if find_program('gtkdoc-scan', required : false).found()
    gtkdoc_enabled = true
    subdir('docs')
  else
    message('Not building documentation as gtk-doc was not found')
  endif
endif

configure_file(input : 'config.h.in',
              output : 'config.h',
              configuration : conf)


message(''.join([
'nemo-@0@'.format(meson.project_version()),
'''

    prefix:                 @0@'''.format(prefix),
'''
    source code location:   @0@'''.format(meson.source_root()),
'''
    compiler:           @0@'''.format(meson.get_compiler('c').get_id()),
'''
    debugging support:  @0@'''.format(buildtype),
'''

    libexif support:    @0@
    exempi  support:    @1@
    Tracker support:    @2@'''.format(libexif_enabled, exempi_enabled, tracker_enabled),
'''

    nemo-extension documentation: @0@
    nemo-extension introspection: @1@'''.format(gtkdoc_enabled, 'introspection ?'),
]))


# ## TODO

#     │   └── pkgconfig
#     │       └── libnemo-extension.pc
