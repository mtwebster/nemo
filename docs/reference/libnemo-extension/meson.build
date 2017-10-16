
if gtkdoc_enabled
  docConf = configuration_data()
  docConf.set('NEMO_EXTENSION_VERSION_INFO', NEMO_EXTENSION_VERSION_INFO)
  configure_file(
    input : 'version.xml.in',
    output: 'version.xml',
    configuration: docConf
  )

  gnome.gtkdoc('libnemo-extension',
    main_xml: 'libnemo-extension-docs.xml',
    src_dir: nemo_extension_incdir,
    dependencies: [ nemo_extension ],
    install: true,
    install_dir: 'libnemo-extension',
    scan_args: [ '--deprecated-guards="G_DISABLE_DEPRECATED"' ],
    scanobjs_args: [ '--type-init-func="g_type_init()"', '--ignore-headers="config.h"' ],
    # gobject_typesfile: a list of type files,
    # fixxref_args: [],
    # html_args: a list of arguments to pass to gtkdoc-mkhtml,
    # html_assets: a list of assets for the HTML pages,
    # content_files: a list of content files,
  )

  # MKDB_OPTIONS=--sgml-mode --output-format=xml

  # content_files = \
  #   version.xml \
  #   $(NULL)

endif
