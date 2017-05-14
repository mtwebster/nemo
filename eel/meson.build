
eel_sources = [
    'eel-accessibility.c',
    'eel-art-extensions.c',
    'eel-canvas.c',
    'eel-debug.c',
    'eel-editable-label.c',
    'eel-gdk-extensions.c',
    'eel-glib-extensions.c',
    'eel-gnome-extensions.c',
    'eel-graphic-effects.c',
    'eel-gtk-extensions.c',
    'eel-lib-self-check-functions.c',
    'eel-self-checks.c',
    'eel-stock-dialogs.c',
    'eel-string.c',
    'eel-vfs-extensions.c',
]

eel_headers = [
    'eel-accessibility.h',
    'eel-art-extensions.h',
    'eel-canvas.h',
    'eel-debug.h',
    'eel-editable-label.h',
    'eel-gdk-extensions.h',
    'eel-glib-extensions.h',
    'eel-gnome-extensions.h',
    'eel-graphic-effects.h',
    'eel-gtk-extensions.h',
    'eel-lib-self-check-functions.h',
    'eel-self-checks.h',
    'eel-stock-dialogs.h',
    'eel-string.h',
    'eel-vfs-extensions.h',
    'eel.h',
]

eel_deps = [ gtk, libxml, gail ]

eel_lib = static_library('eel',
    eel_sources,
    dependencies: eel_deps,
    include_directories: [ rootInclude, ],
    install: false,
    c_args: '-DG_LOG_DOMAIN="Eel"'
)

eel = declare_dependency(
    include_directories: include_directories('.'),
    link_with: [ eel_lib ],
    dependencies: eel_deps,
)

eel_check = executable('check-program',
    [ 'check-program.c' ],
    dependencies: eel,
    include_directories: [ rootInclude, ],
    install: false
)
