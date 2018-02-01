
################################################################################
dataConf = configuration_data()
dataConf.set('VERSION', meson.project_version())

autorunFile = configure_file(
  input : 'nemo-autorun-software.desktop.in.in',
  output: 'nemo-autorun-software.desktop',
  configuration: dataConf
)

autostartFile = configure_file(
  input : 'nemo-autostart.desktop.in',
  output: 'nemo-autostart.desktop',
  configuration: dataConf
)

nemoDesktopFile = configure_file(
  input : 'nemo.desktop.in.in',
  output: 'nemo.desktop',
  configuration: dataConf
)

install_data(
  [autorunFile, autostartFile, nemoDesktopFile],
  install_dir: join_paths(['share','applications'])
)

################################################################################
dataConf = configuration_data()
dataConf.set('bindir', join_paths([get_option('prefix'), get_option('bindir')]))

dbusFiles = [
  configure_file(
    input : 'nemo.service.in',
    output: 'nemo.service',
    configuration: dataConf
  ),

  configure_file(
    input : 'nemo.FileManager1.service.in',
    output: 'nemo.FileManager1.service',
    configuration: dataConf
  ),
]

install_data(dbusFiles,
  install_dir: join_paths(['share','dbus-1','services'])
)

################################################################################
# install_data(
#     install_dir: 'gir-1.0')
# install_data(
#     install_dir: 'glib-2.0/schemas')

# install_data(
#     install_dir: 'gtksourceview-2.0')
# install_data(
#     install_dir: 'gtksourceview-3.0')


################################################################################
dataConf = configuration_data()
mimeFile = configure_file(
  input : 'nemo.xml.in',
  output: 'nemo.xml.tmp',
  configuration: dataConf
)

mimeFile_translated = custom_target('nemo.xml',
  input : mimeFile,
  output: 'nemo.xml',
  command: ['intltool-merge', '--quiet', '--xml-style', '--utf8', po_subdir, '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: join_paths(['share','mime','packages']),
  build_by_default: true,
)

################################################################################
# dataConf = configuration_data()
# actionFiles = [
#     configure_file(
#         input : '90_new-workspace.nemo_action.in',
#         output: '90_new-workspace.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : '91_delete-workspace.nemo_action.in',
#         output: '91_delete-workspace.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : '92_show-expo.nemo_action.in',
#         output: '92_show-expo.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : 'add-desklets.nemo_action.in',
#         output: 'add-desklets.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : 'change-background.nemo_action.in',
#         output: 'change-background.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : 'desktop-settings.nemo_action.in',
#         output: 'desktop-settings.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : 'mount-archive.nemo_action.in',
#         output: 'mount-archive.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : 'new-launcher.nemo_action.in',
#         output: 'new-launcher.nemo_action',
#         configuration: dataConf),
#     configure_file(
#         input : 'set-as-background.nemo_action.in',
#         output: 'set-as-background.nemo_action',
#         configuration: dataConf),
# ]
# install_data(actionFiles,
#     install_dir: join_paths(['share','nemo','actions']))

################################################################################
install_data([ 'nemo-extras.placeholder', 'nemo-suggested.placeholder' ],
    install_dir: join_paths(['share','nemo']))

################################################################################
dataConf = configuration_data()
rootPolicyFile = configure_file(
    input : '../src/org.nemo.root.policy.in',
    output: 'org.nemo.root.policy',
    configuration: dataConf)

install_data(rootPolicyFile,
    install_dir: join_paths(['share','polkit-1','actions']))

################################################################################

subdir('icons')
