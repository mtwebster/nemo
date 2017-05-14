

test_nemo_search_engine = executable('test-nemo-search-engine',
    [ 'test-nemo-search-engine.c' ],
    include_directories: [ rootInclude, ],
    dependencies: [ gtk, nemo_private ],
    # c_args : nemo_definitions,
)

test_nemo_directory_async = executable('test-nemo-directory-async',
    [ 'test-nemo-directory-async.c' ],
    include_directories: [ rootInclude, ],
    dependencies: [ gtk, nemo_private ],
)

# test_nemo_copy = executable('test-nemo-copy',
#     [ 'test-copy.c', 'test.c' ],
#     include_directories: [ rootInclude, ],
#     dependencies: [ gtk ],
#     link_with: [ libnemo_private ],
# )

# test_eel_editable_label = executable('test-eel-editable-label')
