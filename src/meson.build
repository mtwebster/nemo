


dbusBuiltSources = gnome.gdbus_codegen(
    'nemo-freedesktop-generated',
    join_paths([meson.source_root(), 'data', 'freedesktop-dbus-interfaces.xml']),
    interface_prefix: 'org.freedesktop.',
    namespace: 'NemoFreedesktop',
    object_manager: true)
nemoBuiltSources = gnome.gdbus_codegen(
    'nemo-cinnamon-dbus',
    join_paths([meson.source_root(), 'data', 'org.Cinnamon.xml']),
    interface_prefix: 'org.freedesktop.',
    namespace: 'Nemo',
    object_manager: true,
    annotations: ['org.Cinnamon', 'org.gtk.GDBus.C.Name', 'CinnamonDBus'],
)

gnomeBuiltSources= gnome.compile_resources(
    'nemo-resources', 'nemo.gresource.xml',
    source_dir: '.',
    c_name: 'nemo',
    # export: true, # (Added 0.37.0) if true, export the symbols of the generated sources
    install_header: false,
    install: false)


nemoCommon_sources = [
    dbusBuiltSources,
    gnomeBuiltSources,
    nemoBuiltSources,
    'nemo-action-config-widget.c',
    'nemo-application.c',
    'nemo-blank-desktop-window.c',
    'nemo-bookmark-list.c',
    'nemo-bookmarks-window.c',
    'nemo-config-base-widget.c',
    'nemo-connect-server-dialog-nonmain.c',
    'nemo-connect-server-dialog.c',
    'nemo-connect-server-operation.c',
    'nemo-desktop-icon-grid-view.c',
    'nemo-desktop-item-properties.c',
    'nemo-desktop-manager.c',
    'nemo-desktop-window.c',
    'nemo-error-reporting.c',
    'nemo-extension-config-widget.c',
    'nemo-file-management-properties.c',
    'nemo-floating-bar.c',
    'nemo-freedesktop-dbus.c',
    'nemo-icon-view-container.c',
    'nemo-icon-view-grid-container.c',
    'nemo-icon-view.c',
    'nemo-image-properties-page.c',
    'nemo-interesting-folder-bar.c',
    'nemo-list-model.c',
    'nemo-list-view.c',
    'nemo-location-bar.c',
    'nemo-location-entry.c',
    'nemo-mime-actions.c',
    'nemo-navigation-action.c',
    'nemo-navigation-state.c',
    'nemo-notebook.c',
    'nemo-pathbar.c',
    'nemo-places-sidebar.c',
    'nemo-plugin-manager.c',
    'nemo-previewer.c',
    'nemo-progress-info-widget.c',
    'nemo-progress-ui-handler.c',
    'nemo-properties-window.c',
    'nemo-query-editor.c',
    'nemo-script-config-widget.c',
    'nemo-self-check-functions.c',
    'nemo-statusbar.c',
    'nemo-thumbnail-problem-bar.c',
    'nemo-toolbar.c',
    'nemo-trash-bar.c',
    'nemo-tree-sidebar-model.c',
    'nemo-tree-sidebar.c',
    'nemo-view-dnd.c',
    'nemo-view-factory.c',
    'nemo-view.c',
    'nemo-window-bookmarks.c',
    'nemo-window-manage-views.c',
    'nemo-window-menus.c',
    'nemo-window-pane.c',
    'nemo-window-slot-dnd.c',
    'nemo-window-slot.c',
    'nemo-window.c',
    'nemo-x-content-bar.c',
]

nemoCommon_headers = [
    'nemo-action-config-widget.h',
    'nemo-actions.h',
    'nemo-application.h',
    'nemo-blank-desktop-window.h',
    'nemo-bookmark-list.h',
    'nemo-bookmarks-window.h',
    'nemo-config-base-widget.h',
    'nemo-connect-server-dialog.h',
    'nemo-connect-server-operation.h',
    'nemo-desktop-icon-grid-view.h',
    'nemo-desktop-item-properties.h',
    'nemo-desktop-manager.h',
    'nemo-desktop-window.h',
    'nemo-error-reporting.h',
    'nemo-extension-config-widget.h',
    'nemo-file-management-properties.h',
    'nemo-floating-bar.h',
    'nemo-freedesktop-dbus.h',
    'nemo-icon-view-container.h',
    'nemo-icon-view-grid-container.h',
    'nemo-icon-view.h',
    'nemo-image-properties-page.h',
    'nemo-interesting-folder-bar.h',
    'nemo-list-model.h',
    'nemo-list-view-private.h',
    'nemo-list-view.h',
    'nemo-location-bar.h',
    'nemo-location-entry.h',
    'nemo-mime-actions.h',
    'nemo-navigation-action.h',
    'nemo-navigation-state.h',
    'nemo-notebook.h',
    'nemo-pathbar.h',
    'nemo-places-sidebar.h',
    'nemo-plugin-manager.h',
    'nemo-previewer.h',
    'nemo-progress-info-widget.h',
    'nemo-progress-ui-handler.h',
    'nemo-properties-window.h',
    'nemo-query-editor.h',
    'nemo-script-config-widget.h',
    'nemo-self-check-functions.h',
    'nemo-statusbar.h',
    'nemo-thumbnail-problem-bar.h',
    'nemo-toolbar.h',
    'nemo-trash-bar.h',
    'nemo-tree-sidebar-model.h',
    'nemo-tree-sidebar.h',
    'nemo-view-dnd.h',
    'nemo-view-factory.h',
    'nemo-view.h',
    'nemo-window-bookmarks.h',
    'nemo-window-manage-views.h',
    'nemo-window-menus.h',
    'nemo-window-pane.h',
    'nemo-window-private.h',
    'nemo-window-slot-dnd.h',
    'nemo-window-slot.h',
    'nemo-window-types.h',
    'nemo-window.h',
    'nemo-x-content-bar.h',
]


nemoWindow_sources = [
    'nemo-main-application.c',
    'nemo-main.c',
]
nemoWindow_headers = [
    'nemo-main-application.h',
]

nemoDesktop_sources = [
    'nemo-desktop-application.c',
    'nemo-desktop-icon-view.c',
    'nemo-desktop-main.c',
]
nemoDesktop_headers = [
    'nemo-desktop-application.h',
    'nemo-desktop-icon-view.h',
]


if enableEmptyView
    nemoCommon_sources += 'nemo-empty-view.c'
    nemoCommon_headers += 'nemo-empty-view.h'
endif

nemo_deps = [ cinnamon, exempi, gail, glib, gtk, libexif, libnotify, libxml, math, x11,
    egg, nemo_extension, nemo_private, xapp ]


nemo = executable('nemo',
    nemoCommon_sources + nemoWindow_sources,
    include_directories : [ rootInclude ],
    c_args : nemo_definitions,
    dependencies : nemo_deps,
    install: true
)

nemoDesktop = executable('nemo-desktop',
    nemoCommon_sources + nemoDesktop_sources,
    include_directories : [ rootInclude],
    c_args : nemo_definitions,
    dependencies : nemo_deps,
    install: true
)

nemo_autorun_software = executable('nemo-autorun-software',
    [ 'nemo-autorun-software.c' ],
    include_directories : [ rootInclude, ],
    c_args : nemo_definitions,
    dependencies : nemo_deps,
    install: true
)

nemo_connect_server = executable('nemo-connect-server',
    [
    'nemo-bookmark-list.c',
    'nemo-connect-server-dialog.c',
    'nemo-connect-server-dialog-main.c',
    'nemo-connect-server-operation.c'
    ],
    include_directories : [ rootInclude, ],
    c_args : nemo_definitions,
    dependencies : nemo_deps,
    install: true
)

nemo_open_with = executable('nemo-open-with',
    [ 'nemo-open-with-main.c' ],
    include_directories : [ rootInclude, ],
    c_args : nemo_definitions,
    dependencies : nemo_deps,
    install: true
)

# LibExecs
nemo_convert_metadata = executable('nemo-convert-metadata',
    [ 'nemo-convert-metadata.c' ],
    include_directories : [ rootInclude, ],
    c_args : nemo_definitions,
    dependencies : nemo_deps,
    install: true,
    install_dir: libExecPath,
)

nemo_extensions_list = executable('nemo-extensions-list',
    [ 'nemo-extensions-list.c' ],
    include_directories: [ rootInclude, ],
    c_args: nemo_definitions,
    dependencies: [ nemo_deps ],
    install: true,
    install_dir: libExecPath,
)
