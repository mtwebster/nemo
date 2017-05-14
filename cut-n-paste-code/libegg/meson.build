
egg_sources = [ 'eggtreemultidnd.c' ]
egg_headers = [ 'eggtreemultidnd.h' ]

egg_deps = [ gtk ]

egg_lib = static_library('egg',
    egg_sources,
    dependencies: egg_deps,
    install: false
)

egg = declare_dependency(
    include_directories: include_directories('..'),
    link_with: [ egg_lib ],
    dependencies: egg_deps,
)
