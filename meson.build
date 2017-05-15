# Meson build file

# https://github.com/linuxmint/nemo
project('nemo', 'c', version : '3.4.1',
  meson_version : '>=0.40.0')

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
  'X11/XF86keysym.h',
]

foreach h : check_headers
  conf.set10('HAVE_' + h.underscorify().to_upper(), compiler.has_header(h))
endforeach

conf.set10('HAVE_MALLOPT', compiler.has_function('mallopt', prefix : '#include <malloc.h>'))

################################################################################
# Translation

conf.set_quoted('GETTEXT_PACKAGE', 'nemo')
conf.set('ENABLE_NLS', compiler.has_header('libintl.h'))
conf.set('HAVE_GETTEXT', true)
conf.set('HAVE_LOCALE_H', compiler.has_header('locale.h'))
conf.set_quoted('LOCALE_DIR',  join_paths([ get_option('prefix'), get_option('localedir') ]))
conf.set_quoted('LOCALEDIR',   join_paths([ get_option('prefix'), get_option('localedir') ]))


################################################################################
add_global_arguments('-Wno-deprecated-declarations', language : 'c')

################################################################################
# Arguments and dependencies checking
gnome = import('gnome')

# libexif
libexif_enabled = get_option('exif')
conf.set('HAVE_EXIF', libexif_enabled)

if libexif_enabled
  libexif = dependency('libexif', version : '>=0.6.20')
endif

# exempi checking
exempi_enabled = get_option('xmp')
conf.set('HAVE_EXEMPI', exempi_enabled)

if exempi_enabled
  exempi = dependency('exempi-2.0', version : '>=2.2.0')
endif

# libselinux
libselinux_enabled = get_option('selinux')
conf.set('HAVE_SELINUX', libselinux_enabled)

if libselinux_enabled
  libselinux = dependency('libselinux', version : '>=2.0')
endif

# Debug Empty View
enableEmptyView = get_option('empty-view')
conf.set10('ENABLE_EMPTY_VIEW', enableEmptyView)

# Check for Tracker
trackerChoice = get_option('enable-tracker')

tracker_enabled = false
if trackerChoice != 'false'
  trackerRequired = (trackerChoice == 'true')
  # Check all the possible versions
    tracker_sparql = dependency('tracker-sparql-1.0',  required : false)
  if not tracker_sparql.found()
    tracker_sparql = dependency('tracker-sparql-0.18', required : false)
  endif
  if not tracker_sparql.found()
    tracker_sparql = dependency('tracker-sparql-0.16', required : trackerRequired)
  endif

  tracker_enabled = trackerRequired or tracker_sparql.found()
endif
conf.set('ENABLE_TRACKER', tracker_enabled)


gtkdoc_enabled = false
if build_machine.system() == 'windows'
  message('Disabling gtk-doc while building on Windows')
else
  if find_program('gtkdoc-scan', required : false).found()
    gtkdoc_enabled = true
  else
    message('Not building documentation as gtk-doc was not found')
  endif
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


################################################################################

rootInclude = include_directories('.')
nemoDataPath      = join_paths([get_option('prefix'), get_option('datadir'), 'nemo'])
libExecPath       = join_paths([get_option('prefix'), get_option('libdir'), 'nemo'])
nemoExtensionPath = join_paths([get_option('prefix'), get_option('libdir'), 'nemo', 'extensions-3.0'])

nemo_definitions =  [
  '-DNEMO_DATADIR="' + nemoDataPath + '"',
  '-DNEMO_EXTENSIONDIR="' + nemoExtensionPath + '"',
  '-DLIBEXECDIR="' + libExecPath + '"',
]

subdir('cut-n-paste-code/libegg')
subdir('data')
subdir('eel')
subdir('files')
subdir('libnemo-extension')
subdir('libnemo-private')
# subdir('po')
subdir('src')
subdir('test')

if gtkdoc_enabled
    subdir('docs')
endif


configure_file(input : 'config.h.meson.in',
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
